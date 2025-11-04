#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    bool processed = false;
};

struct ProblemStatus {
    bool solved = false;
    int solveTime = 0;
    int wrongAttempts = 0;
    int frozenWrongAttempts = 0;
    int frozenSubmissions = 0;
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int solvedCount = 0;
    int penalty = 0;
    vector<int> solveTimes;
    int ranking = 0;
};

bool started = false;
bool frozen = false;
int duration;
int problemCount;
vector<string> problemNames;
map<string, Team> teams;
vector<string> teamOrder;

bool compareTeams(const string& a, const string& b) {
    Team& ta = teams[a];
    Team& tb = teams[b];
    
    if (ta.solvedCount != tb.solvedCount) {
        return ta.solvedCount > tb.solvedCount;
    }
    
    if (ta.penalty != tb.penalty) {
        return ta.penalty < tb.penalty;
    }
    
    vector<int> timesA = ta.solveTimes;
    vector<int> timesB = tb.solveTimes;
    sort(timesA.rbegin(), timesA.rend());
    sort(timesB.rbegin(), timesB.rend());
    
    int minSize = min(timesA.size(), timesB.size());
    for (int i = 0; i < minSize; i++) {
        if (timesA[i] != timesB[i]) {
            return timesA[i] < timesB[i];
        }
    }
    
    return a < b;
}

void updateRankings() {
    sort(teamOrder.begin(), teamOrder.end(), compareTeams);
    for (int i = 0; i < teamOrder.size(); i++) {
        teams[teamOrder[i]].ranking = i + 1;
    }
}

void printScoreboard() {
    for (const string& teamName : teamOrder) {
        Team& team = teams[teamName];
        cout << teamName << " " << team.ranking << " " << team.solvedCount << " " << team.penalty;
        
        for (const string& prob : problemNames) {
            cout << " ";
            ProblemStatus& ps = team.problems[prob];
            
            if (ps.solved) {
                if (ps.wrongAttempts > 0) {
                    cout << "+" << ps.wrongAttempts;
                } else {
                    cout << "+";
                }
            } else if (ps.frozenSubmissions > 0) {
                if (ps.frozenWrongAttempts > 0) {
                    cout << "-" << ps.frozenWrongAttempts << "/" << ps.frozenSubmissions;
                } else {
                    cout << "0/" << ps.frozenSubmissions;
                }
            } else if (ps.wrongAttempts > 0) {
                cout << "-" << ps.wrongAttempts;
            } else {
                cout << ".";
            }
        }
        cout << "\n";
    }
}

int main() {
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams.find(teamName) != teams.end()) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                Team team;
                team.name = teamName;
                teams[teamName] = team;
                teamOrder.push_back(teamName);
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string tmp;
            iss >> tmp >> duration >> tmp >> problemCount;
            
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                started = true;
                for (int i = 0; i < problemCount; i++) {
                    problemNames.push_back(string(1, 'A' + i));
                }
                updateRankings();
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            
            Submission sub;
            sub.problem = problem;
            sub.status = status;
            sub.time = time;
            sub.processed = false;
            
            teams[teamName].submissions.push_back(sub);
            
            if (!frozen) {
                ProblemStatus& ps = teams[teamName].problems[problem];
                
                if (!ps.solved) {
                    if (status == "Accepted") {
                        ps.solved = true;
                        ps.solveTime = time;
                        teams[teamName].solvedCount++;
                        teams[teamName].penalty += 20 * ps.wrongAttempts + time;
                        teams[teamName].solveTimes.push_back(time);
                    } else {
                        ps.wrongAttempts++;
                    }
                }
                teams[teamName].submissions.back().processed = true;
            } else {
                ProblemStatus& ps = teams[teamName].problems[problem];
                if (!ps.solved) {
                    ps.frozenSubmissions++;
                }
            }
        } else if (cmd == "FLUSH") {
            updateRankings();
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen = true;
                for (auto& p : teams) {
                    for (auto& prob : p.second.problems) {
                        prob.second.frozenWrongAttempts = prob.second.wrongAttempts;
                        prob.second.frozenSubmissions = 0;
                    }
                }
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                
                updateRankings();
                printScoreboard();
                
                // Collect all frozen submissions
                vector<pair<int, pair<string, int>>> frozenSubs;
                for (auto& p : teams) {
                    for (int i = 0; i < p.second.submissions.size(); i++) {
                        if (!p.second.submissions[i].processed) {
                            frozenSubs.push_back({p.second.submissions[i].time, {p.first, i}});
                        }
                    }
                }
                
                sort(frozenSubs.begin(), frozenSubs.end());
                
                // Track ranking changes
                vector<pair<string, pair<string, pair<int, int>>>> rankChanges;
                
                for (auto& fs : frozenSubs) {
                    string teamName = fs.second.first;
                    int subIdx = fs.second.second;
                    Team& team = teams[teamName];
                    Submission& sub = team.submissions[subIdx];
                    
                    ProblemStatus& ps = team.problems[sub.problem];
                    
                    if (!ps.solved) {
                        int oldRank = team.ranking;
                        
                        if (sub.status == "Accepted") {
                            ps.solved = true;
                            ps.solveTime = sub.time;
                            team.solvedCount++;
                            team.penalty += 20 * ps.wrongAttempts + sub.time;
                            team.solveTimes.push_back(sub.time);
                            
                            updateRankings();
                            
                            if (team.ranking < oldRank) {
                                // Find which team was at this ranking before
                                for (const string& other : teamOrder) {
                                    if (teams[other].ranking == oldRank && other != teamName) {
                                        rankChanges.push_back({teamName, {other, {team.solvedCount, team.penalty}}});
                                        break;
                                    }
                                }
                            }
                        } else {
                            ps.wrongAttempts++;
                        }
                    }
                    
                    sub.processed = true;
                }
                
                // Sort ranking changes by final rank
                sort(rankChanges.begin(), rankChanges.end(), [](const auto& a, const auto& b) {
                    return teams[a.first].ranking < teams[b.first].ranking;
                });
                
                for (const auto& rc : rankChanges) {
                    cout << rc.first << " " << rc.second.first << " " << rc.second.second.first << " " << rc.second.second.second << "\n";
                }
                
                // Clear frozen submission counts
                for (auto& p : teams) {
                    for (auto& prob : p.second.problems) {
                        prob.second.frozenSubmissions = 0;
                    }
                }
                
                printScoreboard();
                frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            
            if (teams.find(teamName) == teams.end()) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                cout << teamName << " NOW AT RANKING " << teams[teamName].ranking << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, problem, and_, status;
            iss >> teamName >> where;
            
            getline(iss, line);
            size_t probPos = line.find("PROBLEM=");
            size_t statusPos = line.find("STATUS=");
            
            problem = line.substr(probPos + 8, statusPos - probPos - 13);
            status = line.substr(statusPos + 7);
            
            if (teams.find(teamName) == teams.end()) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                
                Submission* found = nullptr;
                for (int i = teams[teamName].submissions.size() - 1; i >= 0; i--) {
                    Submission& sub = teams[teamName].submissions[i];
                    bool probMatch = (problem == "ALL" || sub.problem == problem);
                    bool statusMatch = (status == "ALL" || sub.status == status);
                    
                    if (probMatch && statusMatch) {
                        found = &sub;
                        break;
                    }
                }
                
                if (found) {
                    cout << teamName << " " << found->problem << " " << found->status << " " << found->time << "\n";
                } else {
                    cout << "Cannot find any submission.\n";
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }
    
    return 0;
}
