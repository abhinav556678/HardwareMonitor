// =============================================================================
// FILE: LoginModule.h
// PURPOSE: Encapsulates user authentication logic.
//          Demonstrates ENCAPSULATION — all credential data is private and
//          exposed only via the authenticate() method.
// =============================================================================

#pragma once
#ifndef LOGIN_MODULE_H
#define LOGIN_MODULE_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <iomanip>
#include <limits>

// =============================================================================
// [ENCAPSULATION] LoginModule hides the user database behind a private map.
//                 External code can only call authenticate() — it cannot
//                 read or modify the stored credentials directly.
// =============================================================================
class LoginModule {
    // -- [ENCAPSULATION] Private Data -----------------------------------------
private:
    // Stores username → password pairs (plain-text for demo; use bcrypt in prod)
    std::map<std::string, std::string> userDatabase;

    // Tracks how many failed attempts the current session has had
    int failedAttempts;

    // Maximum allowed failures before the module locks
    const int MAX_ATTEMPTS = 3;

    // -- Private helper — clears stdin buffer cleanly -------------------------
    void clearInput() const {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // -- Private helper — draws a styled header banner ------------------------
    void drawBanner() const {
        std::cout << "\n";
        std::cout << "  +==========================================================+\n";
        std::cout << "  |          HARDWARE RESOURCE & TASK MONITOR                |\n";
        std::cout << "  |                  Secure Login Portal                     |\n";
        std::cout << "  +==========================================================+\n";
        std::cout << "\n";
    }

public:
    // -- Constructor: seeds the demo user database -----------------------------
    LoginModule() : failedAttempts(0) {
        // [ENCAPSULATION] Direct map insertion is only possible from within
        //                 this class — external code cannot tamper with users.
        userDatabase["admin"]    = "admin123";
        userDatabase["student"]  = "oopproject";
        userDatabase["guest"]    = "guest";
    }

    // -- [ENCAPSULATION] Public Interface: authenticate() ---------------------
    // Returns true if the supplied credentials match the internal database.
    // This is the ONLY controlled gateway to the authentication system.
    // -------------------------------------------------------------------------
    bool authenticate() {
        drawBanner();

        while (failedAttempts < MAX_ATTEMPTS) {
            std::string username, password;

            std::cout << "  Username : ";
            std::cin  >> username;

            std::cout << "  Password : ";
            std::cin  >> password;

            // Look up user in private database
            auto it = userDatabase.find(username);
            if (it != userDatabase.end() && it->second == password) {
                std::cout << "\n  [OK]  Login successful. Welcome, "
                          << username << "!\n";
                std::cout << "  ---------------------------------------------\n";
                return true;
            }

            ++failedAttempts;
            int remaining = MAX_ATTEMPTS - failedAttempts;
            std::cout << "\n  [X]   Invalid credentials. "
                      << remaining << " attempt(s) remaining.\n\n";
        }

        std::cout << "\n  [!]   Account locked after " << MAX_ATTEMPTS
                  << " failed attempts. Exiting.\n\n";
        return false;
    }

    // -- [ENCAPSULATION] Setter: allows adding users at runtime ---------------
    void addUser(const std::string& username,
                 const std::string& password) {
        userDatabase[username] = password;
    }
};

#endif // LOGIN_MODULE_H
