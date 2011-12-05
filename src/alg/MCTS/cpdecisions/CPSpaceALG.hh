#ifndef CPSPACEALG_HH
#define CPSPACEALG_HH

#include "src/alg/MCTS/SpaceALG.hh"
#include "CPDecisionALG.hh"
#include "GecodeSpace.hh"

class CPSpaceALG : public SpaceALG
{
public:
    CPSpaceALG();
    CPSpaceALG(const CPSpaceALG&);
    virtual ~CPSpaceALG();
    CPSpaceALG &operator=(const CPSpaceALG&);
    virtual SpaceALG *clone();

    typedef SpaceALG::DecisionsPool DecisionsPool;
    virtual void addDecision(DecisionALG *);
    virtual DecisionsPool generateDecisions() const;
    virtual bool isSolution() const;
    virtual void setpContext(ContextALG *);
    virtual double evaluate() const;

    virtual uint64_t localsearch(std::vector<int>) const;
    virtual uint64_t localsearch2(std::vector<int>) const;

private:
    void copy(const CPSpaceALG&);

protected:
    GecodeSpace *pGecodeSpace_m;
    std::vector<int> perm_m;
};

#endif
