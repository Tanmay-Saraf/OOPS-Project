# Ransomware-Resilient Database System

A fast, secure data storage system designed specifically to survive and defeat ransomware attacks.

## The Problem
When a ransomware attack hits an organization, the damage happens because traditional databases blindly follow orders. If a hacker's script says "overwrite all user data with random encrypted text," a normal database does exactly that, permanently destroying the original files. Administrators usually only realize they are under attack when it is already too late.

## Our Solution
We built a custom database that removes the ability to "overwrite" anything. Instead of erasing old information when an update happens, our system uses an **append-only architecture**. Every change is simply added as a new entry at the end of a timeline. 

If a hacker successfully encrypts or scrambles the current records, the original data is completely safe. Administrators can simply roll the database back to the exact millisecond before the attack began.

## Key Features

* **Never-Overwrite Architecture:** Data is never deleted or replaced. Every change is tracked as a new version, creating a permanent, indestructible history of your information.
* **Time-Travel Recovery:** If a ransomware attack scrambles your data today, you can instantly hit "undo" and restore the database to exactly how it looked yesterday.
* **Smart Security Tripwires:** The system actively watches for unnatural spikes in data changes or highly randomized, encrypted text. 
* **Automatic Lockdown:** If a tripwire is triggered or a hidden "canary" record is touched by malware, the system instantly physically locks down. It stops accepting new data to prevent further damage, while keeping the system online so administrators can safely extract the historical files.

## How It Works

Our system is divided into four main parts, working together to balance incredible speed with unbreakable security:

1. **Network Guard:** Listens for incoming connections over the network and safely manages multiple users trying to save data at the exact same time without crashing.
2. **Security & Defense Engine:** Inspects every single piece of data before it is saved. If it detects malicious behavior, it acts as a circuit breaker, instantly throwing the entire system into Lockdown Mode.
3. **The Memory Engine:** When the system is running, the active data lives in the computer's fast memory (RAM) so legitimate users can search and retrieve their files instantly without waiting.
4. **Disk Storage & Recovery:** The permanent safety net. It safely logs every single command to the hard drive in real-time. If the power goes out or the system needs to restart, this engine reads the timeline and perfectly rebuilds the database in seconds.