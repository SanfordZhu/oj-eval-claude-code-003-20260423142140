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
    bool has_flushed=false;
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
        has_flushed=true;
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
        vector<int> order(teams.size()); iota(order.begin(), order.end(), 0);
        if(!has_flushed){
            sort(order.begin(), order.end(), [&](int a,int b){ return teams[a].name < teams[b].name; });
            for(size_t i=0;i<order.size();i++) teams[order[i]].ranking = (int)i+1;
        }else{
            sort(order.begin(), order.end(), [&](int a,int b){ return teams[a].ranking < teams[b].ranking; });
        }
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
        has_flushed=true;
        // Build ranking order from last flush
        vector<int> order(teams.size()); iota(order.begin(), order.end(), 0);
        sort(order.begin(), order.end(), [&](int a,int b){ return teams[a].ranking < teams[b].ranking; });
        auto outranks = [&](int A,int B){
            const Team &tA=teams[A], &tB=teams[B];
            if(tA.solved_cnt!=tB.solved_cnt) return tA.solved_cnt>tB.solved_cnt;
            if(tA.penalty!=tB.penalty) return tA.penalty<tB.penalty;
            size_t na=tA.solve_times.size(), nb=tB.solve_times.size();
            size_t mn=max(na, nb);
            for(size_t i=0;i<mn;i++){
                int va = (i<na? tA.solve_times[i]: INT_MIN);
                int vb = (i<nb? tB.solve_times[i]: INT_MIN);
                if(va!=vb) return va<vb;
            }
            return tA.name < tB.name;
        };
        while(true){
            // find lowest-ranked team with frozen problems
            int tid=-1;
            for(int i=(int)order.size()-1;i>=0;i--){
                int u=order[i]; bool any=false;
                for(int j=0;j<m;j++) if(teams[u].p[j].frozen && !teams[u].p[j].solved){ any=true; break; }
                if(any){ tid=u; break; }
            }
            if(tid==-1) break;
            // smallest problem id among frozen
            int pid=-1; for(int j=0;j<m;j++) if(teams[tid].p[j].frozen && !teams[tid].p[j].solved){ pid=j; break; }
            auto &pi = teams[tid].p[pid];
            // gather post-freeze submissions
            vector<Submission> post; int k=pi.frozen_after;
            for(int i=(int)submits[tid].size()-1;i>=0 && (int)post.size()<k;i--) if(submits[tid][i].prob==pid) post.push_back(submits[tid][i]);
            reverse(post.begin(), post.end());
            int wrong_after_before_ac=0; int ac_time=0; bool got_ac=false;
            for(const auto &s: post){
                if(!got_ac){
                    if(s.status==ACC){ got_ac=true; ac_time=s.time; }
                    else wrong_after_before_ac++;
                }
            }
            // apply
            pi.wrong_before = pi.frozen_wrong + wrong_after_before_ac;
            if(got_ac){
                pi.solved=true; pi.solve_time=ac_time;
                teams[tid].solved_cnt++;
                teams[tid].penalty += 20LL*pi.wrong_before + pi.solve_time;
                teams[tid].solve_times.push_back(pi.solve_time);
                sort(teams[tid].solve_times.begin(), teams[tid].solve_times.end(), greater<int>());
            }
            pi.frozen=false; pi.frozen_after=0; pi.frozen_wrong=0;
            // bubble up tid
            int pos = find(order.begin(), order.end(), tid) - order.begin();
            int oldRank = pos+1;
            while(pos>0 && outranks(tid, order[pos-1])){
                int replacedId = order[pos-1];
                cout << teams[tid].name << ' ' << teams[replacedId].name << ' ' << teams[tid].solved_cnt << ' ' << teams[tid].penalty << "\n";
                swap(order[pos], order[pos-1]);
                pos--;
            }
            // refresh rankings in affected range
            for(int i=pos;i<=oldRank-1;i++) teams[order[i]].ranking = i+1;
        }
        // After scrolling ends, lift freeze
        frozen=false;
        // Output scoreboard after scrolling
        // Update rankings fully consistent
        for(size_t i=0;i<order.size();i++) teams[order[i]].ranking = (int)i+1;
        cout.flush();
        printScoreboard();
    }
    void queryRanking(const string& nm){
        auto it=id.find(nm); if(it==id.end()){ cout << "[Error]Query ranking failed: cannot find the team.\n"; return; }
        cout << "[Info]Complete query ranking.\n";
        if(frozen){ cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n"; }
        int rank;
        if(has_flushed){ rank = teams[it->second].ranking; }
        else{
            vector<int> order(teams.size()); iota(order.begin(), order.end(), 0);
            sort(order.begin(), order.end(), [&](int a,int b){ return teams[a].name < teams[b].name; });
            rank = 0; for(size_t i=0;i<order.size();i++) if(order[i]==it->second){ rank=(int)i+1; break; }
        }
        cout << nm << " NOW AT RANKING " << rank << "\n";
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
        else if(cmd=="SUBMIT"){ char prob; string BY, team, WITH, st, ATKW; int tm; cin>>prob>>BY>>team>>WITH>>st>>ATKW>>tm; C.submit(prob, team, st, tm); }
        else if(cmd=="FLUSH"){ C.flush(); }
        else if(cmd=="FREEZE"){ C.freeze(); }
        else if(cmd=="SCROLL"){ C.scroll(); }
        else if(cmd=="QUERY_RANKING"){ string nm; cin>>nm; C.queryRanking(nm); }
        else if(cmd=="QUERY_SUBMISSION"){ string nm, WHERE, PROBLEM, prob, AND, STATUS, st; cin>>nm>>WHERE>>PROBLEM>>prob>>AND>>STATUS>>st; if(prob.find('=')!=string::npos) prob=prob.substr(prob.find('=')+1); if(st.find('=')!=string::npos) st=st.substr(st.find('=')+1); C.querySubmission(nm, prob, st); }
        else if(cmd=="END"){ cout << "[Info]Competition ends.\n"; break; }
    }
    return 0;
}
