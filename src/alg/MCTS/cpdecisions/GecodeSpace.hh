#ifndef GECODESPACE_HH_
#define GECODESPACE_HH_

#include "CPDecisionALG.hh"
#include "bo/ContextBO.hh"
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <vector>

class GecodeSpace: public Gecode::Space
{
public:
    enum BranchMethod {MC, LS};

    GecodeSpace(const ContextBO*, const vector<int>&);
    GecodeSpace(bool, GecodeSpace&);
    virtual Gecode::Space *copy(bool);
    std::vector<int> solution(const vector<int>&);
    GecodeSpace *safeClone();

    // constraints
    void capacity(const ContextBO*, const vector<int>&, Gecode::Matrix<Gecode::BoolVarArgs>&);
    void conflict(const ContextBO*, const vector<int>&);
    void spread(const ContextBO*, const vector<int>&);
    void dependency(const ContextBO*, const vector<int>&);
    void transient(const ContextBO*, const vector<int>&, Gecode::Matrix<Gecode::BoolVarArgs>&);

    // Decision management
    void addDecision(const CPDecisionALG*);
    typedef std::vector<DecisionALG*> DecisionPool;
    DecisionPool generateDecisions();
    bool isSolution();

    // branching
    void postBranching(BranchMethod);

    // LocalSearch
    void restrictNbMove(int, const vector<int>&, const vector<int>&);
    void restrictExceptProc(int, const vector<int>&, const vector<int>&);
    void restrictExceptProcs(const vector<int> vExceptProcs_p, const vector<int>&, const vector<int>&);
    int nbPossibilitiesForProc(int proc_p, const vector<int> &perm_p);

protected:
    // machine[ProcessId] == the machine on which ProcessId is affectd
    Gecode::IntVarArray machine_m;
    // number of assignment equal to the initial solution
    Gecode::IntVar nbUnmovedProcs_m;
    // the boolean matrix proc x mach
    Gecode::BoolVarArray bMatrix_m;
};

#endif //GECODESPACE_HH_
