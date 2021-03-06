/*
 * Copyright (c) 2011 Pierre-Etienne Bougué <pe.bougue(a)gmail.com>
 * Copyright (c) 2011 Florian Colin <florian.colin28(a)gmail.com>
 * Copyright (c) 2011 Kamal Fadlaoui <kamal.fadlaoui(a)gmail.com>
 * Copyright (c) 2011 Quentin Lequy <quentin.lequy(a)gmail.com>
 * Copyright (c) 2011 Guillaume Pinot <guillaume.pinot(a)tremplin-utc.net>
 * Copyright (c) 2011 Cédric Royer <cedroyer(a)gmail.com>
 * Copyright (c) 2011 Guillaume Turri <guillaume.turri(a)gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "tools/Checker.hh"
#include "tools/Log.hh"
#include "alg/ContextALG.hh"
#include "bo/BalanceCostBO.hh"
#include "bo/ContextBO.hh"
#include "bo/LocationBO.hh"
#include "bo/MachineBO.hh"
#include "bo/MMCBO.hh"
#include "bo/ProcessBO.hh"
#include "bo/RessourceBO.hh"
#include "bo/ServiceBO.hh"
#include <boost/foreach.hpp>
#include <set>
#include <vector>
using namespace std;

Checker::Checker(ContextALG const * pContextALG_p) :
    pContextALG_m(pContextALG_p),
    contextToDelete_m(false)
{}

Checker::Checker(ContextBO const * pContextBO_p, const vector<int>& sol_p) :
    pContextALG_m(buildMyContextALG(pContextBO_p, sol_p)),
    contextToDelete_m(true)
{}

Checker::Checker(const Checker& checker_p){
    throw (string("Interdiction d'utiliser le cpy ctr du Checker"));
}

Checker::~Checker(){
    if ( contextToDelete_m ){
        delete pContextALG_m;
    }
}

bool Checker::isValid(){
    return checkCapaIncludingTransient()
        && checkConflict()
        && checkSpread()
        && checkDependances();
}

uint64_t Checker::computeScore(){
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    return computeLoadCost()
        + computeBalanceCost()
        + pContextBO_l->getPoidsPMC() * computePMC()
        + pContextBO_l->getPoidsSMC() * computeSMC()
        + pContextBO_l->getPoidsMMC() * computeMMC();
}

bool Checker::checkCapaIncludingTransient(){
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    const int nbRess_l = pContextBO_l->getNbRessources();

    for ( int idxRess_l=0 ; idxRess_l < nbRess_l ; idxRess_l++ ){
        RessourceBO const * pRess_l = pContextBO_l->getRessource(idxRess_l);
        if ( ! checkCapaIncludingTransient(pRess_l) ){
            return false;
        }
    }
    return true;
}

bool Checker::checkCapaIncludingTransient(RessourceBO const * pRess_p){
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    const vector<int>& currentSol_l = pContextALG_m->getCurrentSol();
    vector<int> vUsedRess_l(pContextBO_l->getNbMachines(), 0);

    const int nbProcess_l = pContextBO_l->getNbProcesses();
    for ( int idxP_l=0 ; idxP_l < nbProcess_l ; idxP_l++ ){
        ProcessBO const * pProcess_l = pContextBO_l->getProcess(idxP_l);
        const int idxCurMachine_l = currentSol_l[idxP_l];
        vUsedRess_l[idxCurMachine_l] += pProcess_l->getRequirement(pRess_p);

        if ( pRess_p->isTransient() ){
            int idxMachineInit_l = pProcess_l->getMachineInit()->getId();
            if ( idxCurMachine_l != idxMachineInit_l ){
                vUsedRess_l[idxMachineInit_l] += pProcess_l->getRequirement(pRess_p);
            }
        }
    }

    const int nbMachines_l = pContextBO_l->getNbMachines();
    for ( int idxMachine_l=0 ; idxMachine_l < nbMachines_l ; idxMachine_l++ ){
        const int capa_l = pContextBO_l->getMachine(idxMachine_l)->getCapa(pRess_p);
        if ( vUsedRess_l[idxMachine_l] > capa_l ){
            LOG(DEBUG) << "La solution viole la contrainte de capa pour la ressource " 
                << pRess_p->getId() << " sur la machine " << idxMachine_l 
                << " (requirement : " << vUsedRess_l[idxMachine_l] << ", capa : " << capa_l << ")" << endl;
            return false;
        }
    }

    return true;
}

bool Checker::checkConflict(){
    set<pair<int, int> > assocesMachineService_l;
    const vector<int>& curSol_l = pContextALG_m->getCurrentSol();
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();

    for ( int idxP_l=0 ; idxP_l < pContextBO_l->getNbProcesses() ; idxP_l++ ){
        const int idxCurMachine_l = curSol_l[idxP_l];
        const int idxService_l = pContextBO_l->getProcess(idxP_l)->getService()->getId();
        pair<int, int> assoceMachineService_l(idxCurMachine_l, idxService_l);
        if ( assocesMachineService_l.find(assoceMachineService_l) != assocesMachineService_l.end() ){
            LOG(DEBUG) << "La solution viole la contrainte de conflit : plusieurs processes du service "
                << idxService_l << " sur la machine " << idxCurMachine_l << endl;
            return false;
        }
        assocesMachineService_l.insert(assoceMachineService_l);
    }

    return true;
}

bool Checker::checkSpread(){
    const vector<int>& curSol_l = pContextALG_m->getCurrentSol();
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    vector<set<int> > memory_l(pContextBO_l->getNbServices());; //memory_l[idxService] => liste des locations utilisees

    for ( int idxP_l=0 ; idxP_l < pContextBO_l->getNbProcesses() ; idxP_l++ ){
        const int idxService_l = pContextBO_l->getProcess(idxP_l)->getService()->getId();
        const int idxLocation_l = pContextBO_l->getMachine(curSol_l[idxP_l])->getLocation()->getId();
        memory_l[idxService_l].insert(idxLocation_l);
    }

    for ( int idxS_l=0 ; idxS_l < pContextBO_l->getNbServices() ; idxS_l++ ){
        const int spreadMin_l = pContextBO_l->getService(idxS_l)->getSpreadMin();
        if ( spreadMin_l > (int) memory_l[idxS_l].size() ){
            LOG(DEBUG) << "La solution viole la contrainte de spread : le service "
                << idxS_l << " s'etend sur " << memory_l[idxS_l].size() 
                << " locations mais a un spread min de " << spreadMin_l << endl;
            return false;
        }
    }

    return true;
}

bool Checker::checkDependances(){
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    const int nbServices_l = pContextBO_l->getNbServices();

    for ( int idxS1_l=0 ; idxS1_l < nbServices_l ; idxS1_l++ ){
        ServiceBO const * pS1_l = pContextBO_l->getService(idxS1_l);
        if ( pS1_l->getServicesIDependOn().empty() ){
            continue;
        }

        unordered_set<int> neighsUsedBy1_l = pContextALG_m->getNeighsUsedByService(pS1_l);

        BOOST_FOREACH(int idxS2_l, pS1_l->getServicesIDependOn()){
            unordered_set<int> neighsUsedBy2_l = pContextALG_m->getNeighsUsedByService(idxS2_l);

            BOOST_FOREACH(int idxN_l, neighsUsedBy1_l){
                if ( neighsUsedBy2_l.find(idxN_l) == neighsUsedBy2_l.end() ){
                    LOG(DEBUG) << "La solution viole la contrainte de dependances : "
                        << "le service " << idxS1_l << " utilise le neighborhood "
                        << idxN_l << " mais depend du service " << idxS2_l
                        << " qui n'y est pas present" << endl;
                    return false;
                }
            }
        }
    }

    return true;
}

uint64_t Checker::computeLoadCost(){
    uint64_t result_l(0);
    ContextBO const * pContextBO_l = pContextALG_m->getContextBO();
    const int nbRess_l = pContextBO_l->getNbRessources();
    for ( int idxRess_l=0 ; idxRess_l < nbRess_l ; idxRess_l++ ){
        int loadCostRess_l = computeLoadCost(idxRess_l);
        result_l += loadCostRess_l;
    }
    LOG(DEBUG) << "Load cost : " << result_l << endl;
    return result_l;
}

uint64_t Checker::computeLoadCost(int idxRess_p){
    uint64_t result_l(0);
    const int nbMachine_l = pContextALG_m->getContextBO()->getNbMachines();
    for ( int idxMachine_l=0 ; idxMachine_l < nbMachine_l ; idxMachine_l++ ){
        result_l += computeLoadCost(idxRess_p, idxMachine_l);
    }
    result_l *= pContextALG_m->getContextBO()->getRessource(idxRess_p)->getWeightLoadCost();
    LOG(USELESS) << "\tload cost pour la ress " << idxRess_p << " : " << result_l << endl;
    return result_l;
}

uint64_t Checker::computeLoadCost(int idxRess_p, int idxMachine_p){
    MachineBO* pMachine_l = pContextALG_m->getContextBO()->getMachine(idxMachine_p);

    int safetyCapa_l = pMachine_l->getSafetyCapa(idxRess_p);
    uint64_t result_l = max(0, pContextALG_m->getRessUsedOnMachine(idxRess_p, idxMachine_p) - safetyCapa_l);
    LOG(USELESS) << "\t\tload cost pour la ress " << idxRess_p << " sur la machine "
        << idxMachine_p << " : " << result_l << endl;
    return result_l;
}

uint64_t Checker::computeBalanceCost(){
    const int nbMachines_l = pContextALG_m->getContextBO()->getNbMachines();
    uint64_t result_l(0);
    for ( int idxMachine_l=0 ; idxMachine_l < nbMachines_l ; idxMachine_l++ ){
        result_l += computeBalanceCost(idxMachine_l);
    }

    return result_l;
}

uint64_t Checker::computeBalanceCost(int idxMachine_p){
    const int nbBalanceCost_l = pContextALG_m->getContextBO()->getNbBalanceCosts();
    uint64_t result_l(0);

    for ( int idxBC_l=0 ; idxBC_l < nbBalanceCost_l ; idxBC_l++ ){
        result_l += computeBalanceCost(idxMachine_p, idxBC_l);
    }

    return result_l;
}

uint64_t Checker::computeBalanceCost(int idxMachine_p, int idxBC_l ){
    BalanceCostBO const * pBC_l = pContextALG_m->getContextBO()->getBalanceCost(idxBC_l);
    RessourceBO* pRess1_l = pBC_l->getRessource1();
    RessourceBO* pRess2_l = pBC_l->getRessource2();
    const int qteRess1Used_l = pContextALG_m->getRessUsedOnMachine(pRess1_l->getId(), idxMachine_p);
    const int qteRess2Used_l = pContextALG_m->getRessUsedOnMachine(pRess2_l->getId(), idxMachine_p);
    const int capaRess1_l = pContextALG_m->getContextBO()->getMachine(idxMachine_p)->getCapa(pRess1_l);
    const int capaRess2_l = pContextALG_m->getContextBO()->getMachine(idxMachine_p)->getCapa(pRess2_l);

    const int a1_l = capaRess1_l - qteRess1Used_l;
    const int a2_l = capaRess2_l - qteRess2Used_l;

    /* FIXME : si on boucle d'abord sur les bc, *puis* sur les machine, on n'aura
     * a effectuer qu'une multiplication par bc et non une par couple (bc, machine)
     * (ceci dit, il peut etre interessant de conserver la possibilite d'estimer les bc sur une machine donnee...)
     */
    return pBC_l->getPoids() * max(0, pBC_l->getTarget()*a1_l - a2_l);
}

uint64_t Checker::computePMC(){
    uint64_t result_l = 0;

    const vector<int>& curSol_l = pContextALG_m->getCurrentSol();
    for ( int idx_l=0 ; idx_l < (int) curSol_l.size() ; idx_l++ ){
        ProcessBO const * pProcess_l = pContextALG_m->getContextBO()->getProcess(idx_l);
        if ( curSol_l[idx_l] != pProcess_l->getMachineInit()->getId() ){
            result_l += pProcess_l->getPMC();
        }
    }

    return result_l;
}

uint64_t Checker::computeSMC(){
    const vector<int>& curSol_l = pContextALG_m->getCurrentSol();
    const int nbProcess_l = curSol_l.size();
    const int nbServices_l = pContextALG_m->getContextBO()->getNbServices();
    vector<int> nbProcessMovedByService_l(nbServices_l, 0);

    for ( int idxP_l=0 ; idxP_l < (int) nbProcess_l ; idxP_l++ ){
        ProcessBO const * pProcess_l = pContextALG_m->getContextBO()->getProcess(idxP_l);
        if ( curSol_l[idxP_l] != pProcess_l->getMachineInit()->getId() ){
            nbProcessMovedByService_l[pProcess_l->getService()->getId()]++;
        }
    }

    uint64_t max_l = 0;
    BOOST_FOREACH(int nbPMoved_l, nbProcessMovedByService_l){
        max_l = max(max_l, (uint64_t) nbPMoved_l);
    }

    return max_l;
}

uint64_t Checker::computeMMC(){
    const vector<int>& curSol_l = pContextALG_m->getCurrentSol();
    const int nbP_l = curSol_l.size();
    MMCBO* pMMCBO_l = pContextALG_m->getContextBO()->getMMCBO();
    uint64_t result_l(0);

    for ( int idxP_l=0 ; idxP_l < nbP_l ; idxP_l++ ){
        result_l += pMMCBO_l->getCost(pContextALG_m->getContextBO()->getProcess(idxP_l)->getMachineInit()->getId() , curSol_l[idxP_l]);
    }

    return result_l;
}

bool check(ContextALG const * pContextALG_p){
    Checker checker_l(pContextALG_p);
    return checker_l.isValid();
}

bool check(ContextBO const * pContextBO_p){
    ContextALG contextALG_l(pContextBO_p);
    return check(&contextALG_l);
}

ContextALG const * Checker::buildMyContextALG(ContextBO const * pContextBO, const vector<int> sol_p){
    ContextALG* pResult_l = new ContextALG(pContextBO, false, false);
    pResult_l->setCurrentSol(sol_p);
    return pResult_l;
}
