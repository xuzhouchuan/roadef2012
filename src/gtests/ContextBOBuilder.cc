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

#include "gtests/ContextBOBuilder.hh"
#include "bo/BalanceCostBO.hh"
#include "bo/ContextBO.hh"
#include "bo/LocationBO.hh"
#include "bo/MachineBO.hh"
#include "bo/MMCBO.hh"
#include "bo/NeighborhoodBO.hh"
#include "bo/ProcessBO.hh"
#include "bo/ServiceBO.hh"

ProcessBO* ContextBOBuilder::buildProcess(int idP_p, ServiceBO* pService_p, const vector<int>& vRequirements_p, int pmc_p, int idxMachineInit_p, ContextBO* pContextBO_p) {
    ProcessBO* pProcess_l = new ProcessBO(idP_p, pService_p, vRequirements_p, pmc_p);
    pProcess_l->setMachineInit(pContextBO_p->getMachine(idxMachineInit_p));
    pContextBO_p->addProcess(pProcess_l);
    return pProcess_l;
}

ServiceBO* ContextBOBuilder::buildService(int idS_p, int spreadMin_p, const unordered_set<int>& sDependances_p, ContextBO* pContextBO_p){
    ServiceBO* pService_l = new ServiceBO(idS_p, spreadMin_p, sDependances_p);
    pContextBO_p->addService(pService_l);
    return pService_l;
}

LocationBO* ContextBOBuilder::buildLocation(int idLoc_p, ContextBO* pContextBO_p){
    LocationBO* pLoc_l = new LocationBO(idLoc_p);
    pContextBO_p->addLocation(pLoc_l);
    return pLoc_l;
}

NeighborhoodBO* ContextBOBuilder::buildNeigh(int idNeigh_p, ContextBO* pContextBO_p){
    NeighborhoodBO* pNeigh_l = new NeighborhoodBO(idNeigh_p);
    pContextBO_p->addNeighborhood(pNeigh_l);
    return pNeigh_l;
}

MachineBO* ContextBOBuilder::buildMachine(int idMachine_p, int idLoc_p, int idNeigh_p, const vector<int>& capa_p, const vector<int>& safetyCapa_p, ContextBO* pContextBO_p){
    for ( int idxLoc_l=pContextBO_p->getNbLocations() ; idxLoc_l <= idLoc_p ; idxLoc_l++ ){
        buildLocation(idxLoc_l, pContextBO_p);
    }

    for ( int idxNeigh_l=pContextBO_p->getNbNeighborhoods() ; idxNeigh_l <= idNeigh_p ; idxNeigh_l++ ){
        buildNeigh(idxNeigh_l, pContextBO_p);
    }

    MachineBO* pMachine_l = new MachineBO(idMachine_p,
            pContextBO_p->getLocation(idLoc_p),
            pContextBO_p->getNeighborhood(idNeigh_p),
            capa_p,
            safetyCapa_p);
    pContextBO_p->addMachine(pMachine_l);
    return pMachine_l;
}

BalanceCostBO* ContextBOBuilder::buildBalanceCost(int idxRess1_p, int idxRess2_p, int target_p, int poids_p, ContextBO* pContextBO_p){
    RessourceBO* pRess1_l = pContextBO_p->getRessource(idxRess1_p);
    RessourceBO* pRess2_l = pContextBO_p->getRessource(idxRess2_p);
    BalanceCostBO* pBC_l = new BalanceCostBO(pRess1_l, pRess2_l, target_p, poids_p);
    pContextBO_p->addBalanceCost(pBC_l);
    return pBC_l;
}

void ContextBOBuilder::buildDefaultMMC(ContextBO* pContextBO_p){
    const int nbMachine_l = pContextBO_p->getNbMachines();
    pContextBO_p->setMMCBO(new MMCBO(vector<vector<int> >(nbMachine_l, vector<int>(nbMachine_l, 0))));
}
