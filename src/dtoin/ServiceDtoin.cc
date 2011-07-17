#include "dtoin/ServiceDtoin.hh"
#include "bo/ContextBO.hh"
#include "bo/ServiceBO.hh"
#include "tools/Log.hh"
#include <cassert>
#include <tr1/unordered_set>
using namespace std;

void ServiceDtoin::read(istream& is_p, ContextBO* pContextBO_p){
    int nbServices_l;
    is_p >> nbServices_l;
    LOG(DEBUG) << nbServices_l << " services" << endl;

    for ( int idxService_l=0 ; idxService_l < nbServices_l ; idxService_l++ ){
        int spreadMin_l, nbDependances_l;
        is_p >> spreadMin_l >> nbDependances_l;
        LOG(DEBUG) << "\tService " << idxService_l << " : spread min=" << spreadMin_l << ", nb dependances=" << nbDependances_l << endl;
        unordered_set<int> dependances_l;

        for ( int idxDep_l=0 ; idxDep_l < nbDependances_l ; idxDep_l++ ){
            int idxServiceDep_l;
            is_p >> idxServiceDep_l;
            dependances_l.insert(idxServiceDep_l);
        }

        pContextBO_p->addService(new ServiceBO(idxService_l, spreadMin_l, dependances_l));
    }
}
