#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H
enum class SecurityState {
    RUNNING,
    SUSPECTED,
    LOCKDOWN
};
class StateManager {
private:
    SecurityState currentState = SecurityState::RUNNING;
public:
    SecurityState getState() const;
    void setState(SecurityState newState);
};

#endif
