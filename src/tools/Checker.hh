#ifndef CHECKER_HH
#define CHECKER_HH
#include <vector>
using namespace std;


class ContextALG;
class ContextBO;
class MachineBO;
class RessourceBO;

/**
 * Fonctions s'appuyant sur la classe #Checker, permettant de s'evitant la creation d'instances temporaires
 */
bool check(ContextALG const * pContextALG_p);
bool check(ContextBO const * pContextBO_p);

class Checker {
    public:
        Checker(ContextALG const * pContextALG_p);

        Checker(ContextBO const * pContextBO_p, const vector<int>& sol_p);

        /**
         * /!\ Ne pas utiliser /!\
         * Defini uniquement afin d'eviter d'en avoir une implantation par defaut.
         * (afin d'eviter d'avoir a perdre du cpu a gerer a vouloir eviter les
         * double delete et/ou fuites, alors qu'on n'a de toute facon pas besoin de ce ctr
         */
        Checker(const Checker& checker_p);

        ~Checker();

        void setContextALG(ContextALG const * pContextALG_p);

        bool isValid();

        /**
         * Calcule le score en supposant que l'instance est valide.
         * Dans le cas contrainte, le comportement est indetermine
         */
        int computeScore();

        /**
         * Methodes permettant de ne checker que quelques contraintes
         */
        bool checkCapaIncludingTransient();
        bool checkConflict();
        bool checkSpread();
        bool checkDependances();
        
        /**
         * Methodes permettant de ne calculer que certains couts
         */
        int computeLoadCost();
        int computeLoadCost(int idxRess_p);
        int computeLoadCost(int idxRess_p, int idxMachine_p);

        int computeBalanceCost();
        int computeBalanceCost(int idxMachine_p);
        int computeBalanceCost(int idxMachine_p, int idxBalanceCost_p);

        int computePMC();
        int computeSMC();
        int computeMMC();

    private:
        bool checkCapaIncludingTransient(RessourceBO const * pRess_p);

        /**
         * Permet de construire un ContextALG customise, et const
         */
        ContextALG const * buildMyContextALG(ContextBO const * pContextBO, const vector<int> sol_p);

        /**
         * Le const permet surtout a la classe cliente de s'assurer qu'on ne pourri
         * pas son contexte. Ceci dit, si le client peut eventuellement la modifier
         * donc attention : il n'est pas possible de cacher quoique se soit !
         */
        ContextALG const * pContextALG_m;

        /**
         * Si le context a ete cree sur mesure par le checker, il faut le deleter nous meme
         */
        bool contextToDelete_m;

};

#endif