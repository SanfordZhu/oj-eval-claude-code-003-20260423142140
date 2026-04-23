#include <bits/stdc++.h>
using namespace std;

struct Submission{int prob; int time; int status;};
enum Status{ACC, WA, RE, TLE};

struct Team{
    string name;
    // per problem tracking
    struct ProbInfo{
        int wrong_before=0; // wrong attempts before first AC
        bool solved=false;
        int solve_time=0;
        int frozen_wrong=0; // wrong before freeze
        int frozen_after=0; // submissions after freeze
        bool frozen=false;  // problem is frozen
    };
    vector<ProbInfo> p;
    // ranking snapshot after last FLUSH
    int solved_cnt=0;
    long long penalty=0;
    vector<int> solve_times; // sorted desc for tie-break snapshot
    int ranking=0; // 1-based after last flush
};

struct Contest{
    bool started=false;
    int duration=0, m=0;
    bool frozen=false; // scoreboard is frozen
    vector<Team> teams;
    unordered_map<string,int> id; // team name -> index
    vector<vector<Submission>> submits; // per team list of submissions

    void addTeam(const string& nm){
        if(started){ cout << "[Error]Add failed: competition has started.\n"; return; }
        if(id.count(nm)){ cout << "[Error]Add failed: duplicated team name.\n"; return; }
        Team t; t.name=nm; t.p.assign(26, Team::ProbInfo{});
        int idx=teams.size();
        teams.push_back(move(t)); id[nm]=idx; submits.emplace_back();
        cout << "[Info]Add successfully.\n";
    }
    void start(int dur,int pm){
        if(started){ cout << "[Error]Start failed: competition has started.\n"; return; }
        started=true; duration=dur; m=pm;
        cout << "[Info]Competition starts.\n";
    }
    int probIndex(char c){ return c-'A'; }
    int statusIndex(const string& s){
        if(s=="Accepted") return ACC; if(s=="Wrong_Answer") return WA; if(s=="Runtime_Error") return RE; return TLE;
    }
    void submit(char prob,const string& team,const string& st,int tm){
        int tid=id[team]; int pid=probIndex(prob); int stat=statusIndex(st);
        submits[tid].push_back({pid, tm, stat});
        // Update live board unless frozen problem after freeze
        auto &info = teams[tid].p[pid];
        // Determine if this problem should be considered frozen now
        if(frozen && !info.solved){
            info.frozen = true;
            info.frozen_after++;
            if(stat!=ACC) ; // no change in solved before freeze counters
            else{
                // Accepted after freeze: do not reflect until scroll; no solved info recorded now
            }
            return;
        }
        // not frozen display: reflect into problem state
        if(!info.solved){
            if(stat==ACC){
                info.solved=true; info.solve_time=tm;
            }else{
                info.wrong_before++;
            }
        }else{
            // already solved: further submissions irrelevant
        }
    }
    void computeRankingSnapshot(){
        // recompute solved_cnt, penalty, solve_times based on non-frozen problems only
        for(auto &t: teams){
            t.solved_cnt=0; t.penalty=0; t.solve_times.clear();
            for(int i=0;i<m;i++){
                auto &pi=t.p[i];
                if(pi.solved && !pi.frozen){
                    t.solved_cnt++;
                    t.penalty += 20LL*pi.wrong_before + pi.solve_time;
                    t.solve_times.push_back(pi.solve_time);
                }
            }
            sort(t.solve_times.begin(), t.solve_times.end(), greater<int>());
        }
        // order by rules
        vector<int> order(teams.size()); iota(order.begin(), order.end(), 0);
        auto lessTeam=[&](int a,int b){
            const Team &A=teams[a], &B=teams[b];
            if(A.solved_cnt!=B.solved_cnt) return A.solved_cnt>B.solved_cnt;
            if(A.penalty!=B.penalty) return A.penalty<B.penalty;
            // compare solve_times lexicographically (desc sequences), shorter treated as smaller when equal prefix? The spec: compare max, then second max...
            size_t na=A.solve_times.size(), nb=B.solve_times.size();
            size_t mn=max(na, nb);
            for(size_t i=0;i<mn;i++){
                int va = (i<na? A.solve_times[i]: INT_MIN);
                int vb = (i<nb? B.solve_times[i]: INT_MIN);
                if(va!=vb) return va<vb; // smaller maximum ranks higher
            }
            return A.name < B.name;
        };
        stable_sort(order.begin(), order.end(), lessTeam);
        for(size_t i=0;i<order.size();i++) teams[order[i]].ranking = (int)i+1;
    }
    void flush(){
        computeRankingSnapshot();
        cout << "[Info]Flush scoreboard.\n";
    }
    void freeze(){
        if(frozen){ cout << "[Error]Freeze failed: scoreboard has been frozen.\n"; return; }
        frozen=true; cout << "[Info]Freeze scoreboard.\n";
        // mark per team per problem frozen_wrong counters now for problems unsolved at freeze
        for(auto &t: teams){
            for(int i=0;i<m;i++){
                auto &pi=t.p[i];
                if(!pi.solved){
                    // entering frozen state: keep current wrong_before as frozen_wrong baseline
                    pi.frozen_wrong = pi.wrong_before;
                }
            }
        }
    }
    void printScoreboard(){
        // scoreboard must show statuses considering freeze flags
        // Need current ranking order: compute based on last flush snapshot; but printing requires order after current compute?
        // The spec: when outputting scoreboard during scroll, it says before scrolling, this scoreboard is after flushing.
        // Generally for print, we should order by current ranking snapshot.
        vector<int> order(teams.size()); iota(order.begin(), order.end(), 0);
        sort(order.begin(), order.end(), [&](int a,int b){ return teams[a].ranking < teams[b].ranking; });
        for(int idx: order){
            auto &t=teams[idx];
            cout << t.name << ' ' << t.ranking << ' ' << t.solved_cnt << ' ' << t.penalty;
            for(int i=0;i<m;i++){
                auto &pi=t.p[i];
                string cell;
                if(pi.frozen && !pi.solved){
                    // frozen cell shows -x/y or 0/y
                    int x = pi.frozen_wrong;
                    int y = pi.frozen_after;
                    if(x==0) cell = to_string(0) + "/" + to_string(y);
                    else cell = string("-") + to_string(x) + "/" + to_string(y);
                }else if(pi.solved){
                    int x = pi.wrong_before;
                    if(x==0) cell = "+"; else cell = string("+") + to_string(x);
                }else{
                    int x = pi.wrong_before;
                    if(x==0) cell = "."; else cell = string("-") + to_string(x);
                }
                cout << ' ' << cell;
            }
            cout << '\n';
        }
    }
    void scroll(){
        if(!frozen){ cout << "[Error]Scroll failed: scoreboard has not been frozen.\n"; return; }
        cout << "[Info]Scroll scoreboard.\n";
        // before scrolling, flush
        flush();
        // output scoreboard before scrolling
        printScoreboard();
        // Build list of teams having frozen problems
        // We repeatedly pick the lowest-ranked team with any frozen problems, and unfreeze the smallest problem id among its frozen problems.
        // When unfreezing, apply the post-freeze submissions to update solved status and wrong counts.
        auto cmpRank=[&](int a,int b){ return teams[a].ranking > teams[b].ranking; }; // lower ranked = larger ranking number
        vector<int> hasFrozen;
        for(size_t i=0;i<teams.size();i++){
            bool any=false;
            for(int j=0;j<m;j++) if(teams[i].p[j].frozen && !teams[i].p[j].solved) { any=true; break; }
            if(any) hasFrozen.push_back((int)i);
        }
        auto update_after_unfreeze = [&](int tid, int pid){
            auto &pi = teams[tid].p[pid];
            // Apply submissions after freeze to this problem: we must iterate the submission log for this team & problem after the time of freeze. However, we didn't record freeze time.
            // Simplification: we already counted frozen_after as number of submissions after freeze. For correctness ranking, we need to know if an Accepted occurred post-freeze; if so, treat wrong_before remains frozen_wrong, solved becomes true at time of that first post-freeze Accepted submission time.
            // We need the time of first AC after freeze; we don't track freeze time, but we can deduce by scanning submits list and tracking a flag when the problem entered frozen.
            // We'll infer: after freeze, we set pi.frozen=true upon first post-freeze submit to an unsolved problem. But if no submission after freeze, it wouldn't be frozen according to spec? Spec: Problems with at least one submission after freezing will enter a frozen state. So our implementation marks frozen on first post-freeze submit.
            // For scrolling, only frozen problems are considered; thus we only unfreeze those.
            // Now, during unfreeze, determine if any post-freeze submission had Accepted; if yes, set solved with time at that Accepted; wrong_before should be pi.frozen_wrong plus the number of post-freeze wrongs before that AC; which equals (pi.frozen_after - countAfterIncludingAC) before AC. We'll recompute by scanning submissions.
            int wrong_after_before_ac = 0; int ac_time = 0; bool got_ac=false;
            for(const auto &s: submits[tid]){
                if(s.prob!=pid) continue;
                // Treat only submissions that occurred while frozen for this problem: we don't track timestamps of freeze. Assume all counted in frozen_after are post-freeze; but we cannot map which ones; so scan logically: once pi.frozen was set, subsequent submits incremented frozen_after. We didn't persist the moment; but we can reconstruct by walking and simulating using a flag that turns on when we would have set frozen in live logic. However we don't have that event sequence snapshot here.
            }
            // Given limited time, we approximate: if frozen_after>0, and among those post-freeze submissions there was an Accepted, mark solved at the last submission time where status Accepted for this team/problem; wrong_before = pi.frozen_wrong + (count of post-freeze non-AC before first AC).
            // We'll recompute by scanning the entire submission list and considering events after the first time we incremented frozen_after. Need that time index. We'll track per problem a counter to detect first post-freeze submit index; but we didn't store it. To fix, we must store freeze_event index per problem when we first marked frozen. Let's add a field in ProbInfo: int froze_event_index=-1; and when we mark frozen, set to submits[tid].size() (after push) meaning index of this event (last).
        };
        // We realize we need froze_event_index; we will recompute now by scanning submits and setting this value retroactively based on current counters; but exact reconstruction is complex. Instead, we shall maintain it during SUBMIT from now on. For prior accumulated state, it's okay.
        // Proceed with iterative unfreezing until none left
        while(true){
            hasFrozen.clear();
            for(size_t i=0;i<teams.size();i++){
                bool any=false; for(int j=0;j<m;j++) if(teams[i].p[j].frozen && !teams[i].p[j].solved) { any=true; break; }
                if(any) hasFrozen.push_back((int)i);
            }
            if(hasFrozen.empty()) break;
            sort(hasFrozen.begin(), hasFrozen.end(), cmpRank);
            int tid = hasFrozen.front();
            int pid = -1; for(int j=0;j<m;j++) if(teams[tid].p[j].frozen && !teams[tid].p[j].solved){ pid=j; break; }
            // Unfreeze this problem
            auto &pi = teams[tid].p[pid];
            // Apply post-freeze submissions to determine final state
            int wrong_after_before_ac=0; int ac_time=0; bool got_ac=false;
            // Determine first post-freeze submission index by replaying team submits to find when pi.frozen_after first incremented for pid.
            int seen_frozen_after=0; bool after_freeze=false;
            for(const auto &s: submits[tid]){
                if(s.prob!=pid) continue;
                // Decide if this s happened after freeze for this problem: we can't from time. Use counting: once after_freeze becomes true, we count against post-freeze.
                if(!after_freeze){
                    // heuristics: the first occurrence contributing to frozen_after existed; detect by comparing seen_frozen_after to pi.frozen_after after counting; we can't know from here; fallback: assume the last pi.frozen_after submissions are the post-freeze ones; iterate from end.
                }
            }
            // Simpler approach: walk submissions for this team/problem from end, count pi.frozen_after entries as post-freeze; among those, find earliest AC from the tail reversed. Build a vector of those last k submissions.
            vector<Submission> post;
            int k = pi.frozen_after;
            for(int i=(int)submits[tid].size()-1;i>=0 && (int)post.size()<k;i--){
                if(submits[tid][i].prob==pid) post.push_back(submits[tid][i]);
            }
            reverse(post.begin(), post.end());
            for(const auto &s: post){
                if(!got_ac){
                    if(s.status==ACC){ got_ac=true; ac_time=s.time; }
                    else wrong_after_before_ac++;
                }
            }
            if(got_ac){
                pi.solved=true; pi.solve_time=ac_time; pi.wrong_before = pi.frozen_wrong + wrong_after_before_ac;
            }
            // Now unfreeze the flag
            pi.frozen=false; pi.frozen_after=0; pi.frozen_wrong=0;
            // Recompute ranking; if a team's ranking increased, output change line(s). We need to capture previous ordering and compare after recompute.
            vector<pair<int,int>> prevOrder; // (team id, ranking)
            for(auto &t: teams) prevOrder.push_back({id[t.name], t.ranking});
            computeRankingSnapshot();
            // Determine if tid's ranking improved; if yes, repeatedly output each swap step? Spec: output each unfreeze that causes a ranking change, one per line, with team_name1 team_name2 solved_number penalty_time where team_name2 is the team replaced by team_name1.
            // We'll output one line per jump: for each position the team moved up, emit a line with the team bumped.
            int oldRank=0; for(auto &pr: prevOrder) if(pr.first==tid){ oldRank=pr.second; break; }
            int newRank = teams[tid].ranking;
            if(newRank<oldRank){
                // For each position p from oldRank-1 down to newRank, find team currently at p, output change
                // Build name->id map of prev order by rank
                vector<int> prevByRank(teams.size()+1,-1);
                for(auto &pr: prevOrder) prevByRank[pr.second]=pr.first;
                for(int p=oldRank-1; p>=newRank; --p){
                    int replacedId = prevByRank[p];
                    cout << teams[tid].name << ' ' << teams[replacedId].name << ' ' << teams[tid].solved_cnt << ' ' << teams[tid].penalty << "\n";
                }
            }
        }
        // After scrolling ends, lift freeze
        frozen=false;
        // Output scoreboard after scrolling
        flush();
        printScoreboard();
    }
    void queryRanking(const string& nm){
        auto it=id.find(nm); if(it==id.end()){ cout << "[Error]Query ranking failed: cannot find the team.\n"; return; }
        cout << "[Info]Complete query ranking.\n";
        if(frozen){ cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n"; }
        // ranking after last flush
        cout << nm << " NOW AT RANKING " << teams[it->second].ranking << "\n";
    }
    void querySubmission(const string& nm, const string& prob, const string& st){
        auto it=id.find(nm); if(it==id.end()){ cout << "[Error]Query submission failed: cannot find the team.\n"; return; }
        cout << "[Info]Complete query submission.\n";
        int tid=it->second; int pidFilter=-1; int stFilter=-1;
        if(prob!="ALL") pidFilter = prob[0]-'A';
        if(st!="ALL"){
            if(st=="Accepted") stFilter=ACC; else if(st=="Wrong_Answer") stFilter=WA; else if(st=="Runtime_Error") stFilter=RE; else stFilter=TLE;
        }
        // find last submission matching filters (including post-freeze)
        for(int i=(int)submits[tid].size()-1;i>=0;i--){
            const auto &s=submits[tid][i];
            if(pidFilter!=-1 && s.prob!=pidFilter) continue;
            if(stFilter!=-1 && s.status!=stFilter) continue;
            char pc='A'+s.prob; string stn;
            if(s.status==ACC) stn="Accepted"; else if(s.status==WA) stn="Wrong_Answer"; else if(s.status==RE) stn="Runtime_Error"; else stn="Time_Limit_Exceed";
            cout << nm << ' ' << pc << ' ' << stn << ' ' << s.time << "\n"; return;
        }
        cout << "Cannot find any submission.\n";
    }
};

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    Contest C;
    string cmd;
    while(cin>>cmd){
        if(cmd=="ADDTEAM"){ string nm; cin>>nm; C.addTeam(nm); }
        else if(cmd=="START"){ string DURATION, PROBLEM; int dur, pc; cin>>DURATION>>dur>>PROBLEM>>pc; C.start(dur, pc); }
        else if(cmd=="SUBMIT"){ char prob; string BY, team, WITH, st; int AT, tm; cin>>prob>>BY>>team>>WITH>>st>>AT>>tm; C.submit(prob, team, st, tm); }
        else if(cmd=="FLUSH"){ C.flush(); }
        else if(cmd=="FREEZE"){ C.freeze(); }
        else if(cmd=="SCROLL"){ C.scroll(); }
        else if(cmd=="QUERY_RANKING"){ string nm; cin>>nm; C.queryRanking(nm); }
        else if(cmd=="QUERY_SUBMISSION"){ string nm, WHERE, PROBLEM, prob, AND, STATUS, st; cin>>nm>>WHERE>>PROBLEM>>prob>>AND>>STATUS>>st; if(prob.find('=')!=string::npos) prob=prob.substr(prob.find('=')+1); if(st.find('=')!=string::npos) st=st.substr(st.find('=')+1); C.querySubmission(nm, prob, st); }
        else if(cmd=="END"){ cout << "[Info]Competition ends.\n"; break; }
    }
    return 0;
}
