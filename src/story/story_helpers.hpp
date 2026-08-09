#pragma once
#include <string>
#include "story_manager.hpp"
#include <debug/logs.hpp>
#include <sstream>  // for std::stringstream

namespace StoryHelpers {
    inline bool evaluateCondition(const std::string& cond) {
        if (cond.empty()) return true;

        // Remove all whitespace for easier parsing
        std::string expr = cond;
        expr.erase(std::remove_if(expr.begin(), expr.end(), ::isspace), expr.end());
        if (expr.empty()) return true;

        // Split by || (OR) – top‑level
        std::vector<std::string> orParts;
        size_t pos = 0;
        std::string remaining = expr;
        while ((pos = remaining.find("||")) != std::string::npos) {
            orParts.push_back(remaining.substr(0, pos));
            remaining.erase(0, pos + 2);
        }
        orParts.push_back(remaining);

        // Evaluate each OR part; if any is true, return true
        for (const auto& part : orParts) {
            // Split by && (AND) within this OR part
            std::vector<std::string> andParts;
            size_t p = 0;
            std::string temp = part;
            while ((p = temp.find("&&")) != std::string::npos) {
                andParts.push_back(temp.substr(0, p));
                temp.erase(0, p + 2);
            }
            andParts.push_back(temp);

            bool andResult = true;
            for (const auto& atom : andParts) {
                // Evaluate one atomic condition (with optional !)
                bool negate = false;
                std::string condStr = atom;
                if (condStr.rfind("!", 0) == 0) {
                    negate = true;
                    condStr = condStr.substr(1);
                }
                bool val = false;
                if (condStr.rfind("hasFlag(", 0) == 0) {
                    std::string key = condStr.substr(8, condStr.size() - 9);
                    val = StoryManager::get().hasFlag(key);
                } else if (condStr.rfind("hasItem(", 0) == 0) {
                    std::string key = condStr.substr(8, condStr.size() - 9);
                    val = StoryManager::get().hasItem(key);
                } else {
                    Log::warn << "Unknown condition atom: " << condStr << std::endl;
                    val = false;
                }
                if (negate) val = !val;
                andResult = andResult && val;
                if (!andResult) break;  // short‑circuit AND
            }
            if (andResult) return true; // OR short‑circuit
        }
        return false;
    }


    inline void executeAction(const std::string& action) {
        if (action.empty()) return;
        std::stringstream ss(action);
        std::string token;
        while (std::getline(ss, token, ';')) {
            // Trim whitespace
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            if (token.empty()) continue;

            if (token.rfind("setFlag(", 0) == 0) {
                std::string key = token.substr(8, token.size() - 9);
                StoryManager::get().setFlag(key);
            } else if (token.rfind("giveItem(", 0) == 0) {
                std::string key = token.substr(9, token.size() - 10);
                StoryManager::get().giveItem(key);
            } else {
                Log::warn << "Unknown action: " << token << std::endl;
            }
        }
    }
}