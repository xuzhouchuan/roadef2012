#ifndef OPPMRESTRICTIONALG_HH
#define OPPMRESTRICTIONALG_HH

#include "src/alg/RestrictionALG.hh"
#include <set>

class OPPMRestrictionALG : public RestrictionALG
{
    public:
        typedef RestrictionALG::ProcessId ProcessId;
        typedef RestrictionALG::ProcessPool ProcessPool;
        
        typedef RestrictionALG::MachineId MachineId;
        typedef RestrictionALG::MachinePool MachinePool;
    
        OPPMRestrictionALG(ProcessId,MachinePool const &);
        ~OPPMRestrictionALG();
        
        void filter(ProcessPool &) const;
        void filter(ProcessId const &, MachinePool &) const;
        
    private:
        ProcessId target_m;

        typedef std::set<MachineId> RestrictedMachinePool;
        RestrictedMachinePool pool_m;
};

#endif