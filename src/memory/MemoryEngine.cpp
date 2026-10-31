#include "memory/MemoryEngine.hpp"
#include <exception>

namespace oops{
    Response MemoryEngine:: execute(const Request& request){
        Response response;
        try{
            switch(request.command){
                case Command::Set:{
                    if(request.key.empty()||request.value.empty()){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message= "Key and value are required";
                        return response;
                    }
                    WalAppendHandler handler;
                    {
                        std::lock_guard<std::mutex> lock(walHandlerMtx);
                        handler=walAppendHandler;
                    }
                    store.put(request.key, request.value, handler);
                    response.status=ResponseStatus::Success;
                    response.message="Key stored successfully";
                    return response;
                }
                
                case Command::Get:{
                    if(request.key.empty()){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message="Key is required";
                        return response;
                    }
                    auto result=store.get(request.key);
                    if(!result.has_value()){
                        response.status= ResponseStatus::NotFound;
                        response.message="Key does not exist";
                        return response;
                    }
                    response.status=ResponseStatus::Success;
                    response.value=result->value;
                    return response;
                }
                case Command::Delete:{
                    if(request.key.empty()){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message="Key is required";
                        return response;
                    }
                    WalAppendHandler handler;
                    {
                    std::lock_guard<std::mutex> lock(walHandlerMtx);
                    handler=walAppendHandler;
                    }
                    bool removed=store.remove(request.key,handler);
                    if(!removed){
                        response.status=ResponseStatus::NotFound;
                        response.message="Key does not exist";
                        return response;
                    }
                    response.status=ResponseStatus::Success;
                    response.message="Key deleted successfully";
                    return response;
                }
                case Command::Exists:{
                    if(request.key.empty()){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message="Key is required";
                        return response;
                    }
                    bool found=store.exists(request.key);
                    response.status=ResponseStatus::Success;
                    response.value=found?"true":"false";
                    return response;
                }
                case Command::Keys:{
                    auto currentKeys=store.keys();
                    response.status=ResponseStatus::Success;
                    for(size_t i=0; i<currentKeys.size(); i++){
                        if(i>0)response.value+=";";
                        response.value+=currentKeys[i];
                    }
                    return response;
                }
                case Command::History:{
                    if(request.key.empty()){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message="Key is required";
                        return response;
                    }
                    auto history=store.getHistory(request.key);
                    if(history.empty()){
                        response.status=ResponseStatus::NotFound;
                        response.message="Key history not found";
                        return response;
                    }
                    response.status=ResponseStatus::Success;
                    for(size_t i=0; i<history.size(); i++){
                        auto timestamp=std::chrono::duration_cast<std::chrono::microseconds>(
                            history[i].timestamp.time_since_epoch()
                        ).count();
                        if(i>0){
                            response.value+=";";
                        }
                        response.value+=std::to_string(history[i].id)+"|";
                        if(history[i].operation==Operation::Put){
                            response.value+="PUT";
                        }else{
                            response.value+="DELETE";
                        }
                        response.value+="|"+std::to_string(timestamp)+"|";
                        response.value+=history[i].value;
                    }
                    return response;
                    
                }  
                case Command::GetVersion:{
                    if(request.key.empty()||request.version==0){
                        response.status=ResponseStatus::InvalidRequest;
                        response.message= "Key and valid version ID are required";
                        return response;
                    }
                    auto version=store.getVersion(request.key, request.version);
                    if(!version.has_value()){
                        response.status=ResponseStatus::NotFound;
                        response.message="Version not found";
                        return response;
                    }
                    auto timestamp=std::chrono::duration_cast<std::chrono::microseconds>(
                            version->timestamp.time_since_epoch()
                        ).count();
                    response.value=std::to_string(version->id)+"|";
                    if(version->operation==Operation::Put){
                        response.value+="PUT";
                    }else{
                        response.value+="DELETE";
                    }
                    response.value+="|"+std::to_string(timestamp)+"|";
                    response.value+=version->value;
                    if(version->operation==Operation::Delete){
                        response.status=ResponseStatus::NotFound;
                    }else{
                        response.status=ResponseStatus::Success;
                    }
                    return response;
                }          
                default:{
                    response.status=ResponseStatus::InvalidRequest;
                    response.message="Command not implemented yet";
                    return response;
                }
            }
        }catch(const std::exception&){
            response.status=ResponseStatus::InternalError;
            response.message="Memory Engine operation failed";
            response.value.clear();
        }
        return response;
    }
    void MemoryEngine::setWalAppendHandler(WalAppendHandler handler){
        std::lock_guard <std::mutex> lock(walHandlerMtx);
        walAppendHandler=handler;
    }
    bool MemoryEngine::replayVersion(const std::string& key, const Version& version){
        return store.replayVersion(key, version);
    }
    void MemoryEngine::clear(){
        store.clear();
    }
}