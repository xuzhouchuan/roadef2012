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

#ifdef USE_GECODE

#include "CPSpaceALG.hh"
#include "alg/ContextALG.hh"
#include "dtoout/SolutionDtoout.hh"
#include "bo/ProcessBO.hh"
#include "bo/MachineBO.hh"
#include "tools/Checker.hh"
#include "tools/Log.hh"

#include <algorithm>
#include <gecode/search.hh>

using namespace Gecode;

CPSpaceALG::CPSpaceALG() : pGecodeSpace_m(0)
{
}

CPSpaceALG::CPSpaceALG(const CPSpaceALG &that) :
    SpaceALG(that), pGecodeSpace_m(0)
{
    copy(that);
}

CPSpaceALG::~CPSpaceALG()
{
    delete pGecodeSpace_m;
}

CPSpaceALG &CPSpaceALG::operator=(const CPSpaceALG &that)
{
    if (this != &that) {
        SpaceALG::operator=(that);
        copy(that);
    }

    return *this;
}

void CPSpaceALG::copy(const CPSpaceALG &that)
{
    delete pGecodeSpace_m;
    pGecodeSpace_m = 0;
    perm_m = that.perm_m;

    if (that.pGecodeSpace_m != 0)
        pGecodeSpace_m = that.pGecodeSpace_m->safeClone();
}

SpaceALG * CPSpaceALG::clone()
{
    return new CPSpaceALG(*this);
}

bool CPSpaceALG::isSolution() const
{
    return pGecodeSpace_m == 0 || pGecodeSpace_m->isSolution();
}

CPSpaceALG::DecisionsPool CPSpaceALG::generateDecisions() const
{
    if (!pGecodeSpace_m)
        return DecisionsPool();
    else
        return pGecodeSpace_m->generateDecisions();
}

void CPSpaceALG::addDecision(DecisionALG *pDecision_p)
{
    assert(dynamic_cast<const CPDecisionALG*>(pDecision_p));

    if (pGecodeSpace_m) {
        const CPDecisionALG *pCPD_l = static_cast<const CPDecisionALG*>(pDecision_p);
        pGecodeSpace_m->addDecision(pCPD_l);
    }
}

struct Comp {
    Comp(const vector<double> &v_p) : size_m(v_p) {}
    const vector<double> &size_m;
    bool operator()(int i_p, int j_p) { return size_m[i_p] < size_m[j_p]; }
};

std::vector<int> permutation(const ContextBO *pContext_p)
{
    int nbProc_l = pContext_p->getNbProcesses();
    int nbRes_l = pContext_p->getNbRessources();
    int nbMach_l = pContext_p->getNbMachines();
    vector<double> resScale_l(nbRes_l);
    vector<double> size_l(nbProc_l);
    vector<int> perm_l(nbProc_l);

    for (int mach_l = 0; mach_l < nbMach_l; ++mach_l) {
        MachineBO *pMach_l = pContext_p->getMachine(mach_l);
        for (int res_l = 0; res_l < nbRes_l; ++res_l)
            resScale_l[res_l] += pMach_l->getCapa(res_l);
    }

    for (int proc_l = 0; proc_l < nbProc_l; ++proc_l) {
        ProcessBO *pProc_l = pContext_p->getProcess(proc_l);
        perm_l[proc_l] = proc_l;
        for (int res_l = 0; res_l < nbRes_l; ++res_l)
            size_l[proc_l] += (double) pProc_l->getRequirement(res_l) / resScale_l[res_l];
    }

    Comp comp_l(size_l);
    sort(perm_l.begin(), perm_l.end(), comp_l);

    return perm_l;
}

void CPSpaceALG::setpContext(ContextALG *pContext_p)
{
    SpaceALG::setpContext(pContext_p);
    delete pGecodeSpace_m;
    perm_m = permutation(pContext_p->getContextBO());
    pGecodeSpace_m = new GecodeSpace(pContext_p->getContextBO(), perm_m);
}

double CPSpaceALG::evaluate() const
{
    double res_l = 0.;

    // si on a pas de Gecode, le pb est FAILED
    if (! pGecodeSpace_m || pGecodeSpace_m->status() == SS_FAILED)
        return res_l;

    // Montecarlo: on demande une solution à Gecode
    Search::FailStop stop_l(100);
    Search::Options options_l;
    options_l.stop = &stop_l;
    options_l.clone = false;
    //options_l.c_d = 1000;
    GecodeSpace *pSpace_l = pGecodeSpace_m->safeClone();
    pSpace_l->postBranching(GecodeSpace::MC);
    DFS<GecodeSpace> search_l(pSpace_l, options_l);
    GecodeSpace *pSol_l = search_l.next();

    // Gecode a pas trouvé de solution réalisable
    if (! pSol_l) {
        if (search_l.stopped()) {
            LOG(INFO) << "Search stopped." << endl;
        }
        return res_l;
    }

    // on récupère la solution
    std::vector<int> sol_l = pSol_l->solution(perm_m);
    delete pSol_l;
    uint64_t eval_l = localsearch(sol_l);
    res_l = (double) origEval_m / ((double) origEval_m + eval_l);

    return res_l;
}

uint64_t CPSpaceALG::localsearch(std::vector<int> bestSol_p) const
{
    bool foundBetter_l = true;
    int nbProc_l = pContext_m->getContextBO()->getNbProcesses();
    int lastImprovedProc_l = 0;
    int aProc_l = nbProc_l - 1;
    Checker checker_l(pContext_m->getContextBO(), bestSol_p);
    assert(checker_l.isValid());
    uint64_t bestEval_l = checker_l.computeScore();
    Search::MemoryStop stop_l(256 * 1024 * 1024);
    Search::Options options_l;
    options_l.stop = &stop_l;
    options_l.c_d = 1;
    options_l.clone = false;

    while (foundBetter_l) {
        foundBetter_l = false;
        GecodeSpace *pSpace_l = pGecodeSpace_m->safeClone();

        // Nombre de mouvement possible = 1
        pSpace_l->restrictNbMove(1, bestSol_p, perm_m);

        pSpace_l->postBranching(GecodeSpace::LS);

        do {
            int proc_l = perm_m[aProc_l];
            GecodeSpace *pCurSpace_l = pSpace_l->safeClone();
            pCurSpace_l->restrictExceptProc(proc_l, bestSol_p, perm_m);

            DFS<GecodeSpace> search_l(pCurSpace_l, options_l);

            GecodeSpace *pSol_l = 0;

            while ((pSol_l = search_l.next()) != 0) {
                std::vector<int> sol_l = pSol_l->solution(perm_m);

                delete pSol_l;
                Checker checker_l(pContext_m->getContextBO(), sol_l);
                assert(checker_l.isValid());
                uint64_t eval_l = checker_l.computeScore();

                if (eval_l < bestEval_l) {
                    bestEval_l = eval_l;
                    bestSol_p = sol_l;
                    foundBetter_l = true;
                    lastImprovedProc_l = aProc_l;
                    if (SolutionDtoout::writeSol(bestSol_p, bestEval_l)) {
                        LOG(INFO) << "Better solution: " << bestEval_l << endl;
                    }
                }
            }

            if (--aProc_l < 0) {
                aProc_l = nbProc_l - 1;
            }
        } while (!foundBetter_l && aProc_l != lastImprovedProc_l);

        delete pSpace_l;
    }

    return bestEval_l;
}

uint64_t CPSpaceALG::localsearch2(std::vector<int> bestSol_p) const
{
    bool foundBetter_l = true;
    int nbProc_l = pContext_m->getContextBO()->getNbProcesses();
    int maxIter_l = 100;
    int lastImprovedProc_l = 0;
    int aProc_l = nbProc_l - 1;
    Checker checker_l(pContext_m->getContextBO(), bestSol_p);
    assert(checker_l.isValid());
    uint64_t bestEval_l = checker_l.computeScore();
    Search::MemoryStop stop_l(256 * 1024 * 1024);
    Search::Options options_l;
    options_l.stop = &stop_l;
    options_l.c_d = 1;
    options_l.clone = false;

    while (foundBetter_l) {
        foundBetter_l = false;
        GecodeSpace *pSpace_l = pGecodeSpace_m->safeClone();
        pSpace_l->postBranching(GecodeSpace::LS);

        do {

            //
            // PARTIE EVALUATION DU NOMBRE DE PROCS A RECALCULER
            //
            vector<int> vProcFree_l;
            int proc1_l = perm_m[aProc_l];
            vProcFree_l.push_back(proc1_l);

            GecodeSpace *pCurSpace_l = pSpace_l->safeClone();
            pCurSpace_l->restrictExceptProcs(vProcFree_l, bestSol_p, perm_m);
            int nbPossibilities_l = pCurSpace_l->nbPossibilitiesForProc(proc1_l, perm_m);

            if( nbPossibilities_l <= maxIter_l ) {

                int idxProcToAdd_l = aProc_l-1;
                while(idxProcToAdd_l >= 0)
                {
                    GecodeSpace *pCurSpace2_l = pSpace_l->safeClone();
                    int procToAdd_l = perm_m[idxProcToAdd_l];
                    vProcFree_l.push_back(procToAdd_l);

                    pCurSpace2_l->restrictExceptProcs(vProcFree_l, bestSol_p, perm_m);

                    // recalcule de la borne max du nombre de mouvements possibles
                    nbPossibilities_l = 1;
                    for(int idxProc=vProcFree_l.size();--idxProc>=0;) {
                        int nbCurrentPoss_l = pCurSpace2_l->nbPossibilitiesForProc(vProcFree_l[idxProc], perm_m);

                        if( nbCurrentPoss_l > 1 ) {
                            nbPossibilities_l *= nbCurrentPoss_l;
                        }
                    }

                    if( nbPossibilities_l > maxIter_l ) {
                        // on depasse la combinatoire toleree, on va resoudre avec ca deja
                        delete pCurSpace2_l;
                        break;
                    }
                    // sinon on continue de chercher
                    delete pCurSpace_l;
                    pCurSpace_l = pCurSpace2_l;
                    --idxProcToAdd_l;
                }
                aProc_l = idxProcToAdd_l+1;
            }

            //
            // FIN PARTIE EVALUATION DU NOMBRE DE PROCS A RECALCULER
            //

            DFS<GecodeSpace> search_l(pCurSpace_l, options_l);

            // enlevage du premier mouvement quand on ne met pas de contrainte sur le nombre
            // de mouvement que doit avoir la solution par rapport a la meilleure solution
            // => le premier mouvement est la solution initiale
            delete search_l.next();

            GecodeSpace *pSol_l = 0;

            while ((pSol_l = search_l.next()) != 0 ) {
                std::vector<int> sol_l = pSol_l->solution(perm_m);

                delete pSol_l;
                Checker checker_l(pContext_m->getContextBO(), sol_l);
                assert(checker_l.isValid());
                uint64_t eval_l = checker_l.computeScore();

                if (eval_l < bestEval_l) {
                    bestEval_l = eval_l;
                    bestSol_p = sol_l;
                    foundBetter_l = true;
                    lastImprovedProc_l = aProc_l;
                    if (SolutionDtoout::writeSol(bestSol_p, bestEval_l)) {
                        LOG(INFO) << "Better solution: " << bestEval_l << endl;
                    }
                }
            }

            if (--aProc_l < 0) {
                break;// la condition d'arrêt a l'air de merder alors je force un tour
                aProc_l = nbProc_l - 1;
            }
        } while (!foundBetter_l && aProc_l != lastImprovedProc_l);

        delete pSpace_l;
    }

    return bestEval_l;
}


#endif //USE_GECODE
