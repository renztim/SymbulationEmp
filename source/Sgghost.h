#ifndef SGGHOST_H
#define SGGHOST_H

// #include "../../Empirical/include/emp/math/Random.hpp"
// #include "../../Empirical/include/emp/tools/string_utils.hpp"
// #include <set>
// #include <iomanip> // setprecision
// #include <sstream> // stringstream
#include "Host.h"
#include "SymWorld.h"


class sggHost: public Host {
protected:
  double sourcepool=0;


public:
  sggHost(emp::Ptr<emp::Random> _random, emp::Ptr<SymWorld> _world, emp::Ptr<SymConfigBase> _config,
  double _intval =0.0, emp::vector<emp::Ptr<Organism>> _syms = {},
  emp::vector<emp::Ptr<Organism>> _repro_syms = {},
  std::set<int> _set = std::set<int>(),
  double _points = 0.0) : Host(_random, _world, _config, _intval,_syms, _repro_syms, _set, _points) { 
    if ( _intval > 1 || _intval < -1) {
       throw "Invalid interaction value. Must be between -1 and 1";  // Exception for invalid interaction value
     };
   }



  sggHost(const sggHost &) = default;
  sggHost(sggHost &&) = default;
  sggHost() = default;
 
  double GetPool() {return sourcepool;}
  void SetPool(double _in) {sourcepool= _in;}
  void AddPool(double _in) {sourcepool += _in;}


  void DistribResources(double resources) {
    double hostIntVal = interaction_val; //using private variable because we can

    //In the event that the host has no symbionts, the host gets all resources not allocated to defense or given to absent partner.
    if(syms.empty()) {

      if(hostIntVal >= 0){
	      double spent = resources * hostIntVal;
        this->AddPoints(resources - spent);
      }
      else {
        double hostDefense = -1.0 * hostIntVal * resources;
        this->AddPoints(resources - hostDefense);
      }
      return; //This concludes resource distribution for a host without symbionts
    }

    //Otherwise, split resources into equal chunks for each symbiont
    int num_sym = syms.size();
    double sym_piece = (double) resources / num_sym;
   

    for(size_t i=0; i < syms.size(); i++){
      double hostPortion = syms[i]->ProcessResources(sym_piece);
      double hostPool = syms[i]->ProcessPool();
      this->AddPoints(hostPortion);
      this->AddPool(hostPool);
    }
    if(syms.size()>0){this->DistribPool();}
    

  } //end DistribResources

  void DistribPool(){
    //to do: marginal return
    int num_sym = syms.size();
    double sym_piece = (double) sourcepool / num_sym;
    for(size_t i=0; i < syms.size(); i++){
        syms[i]->AddPoints(sym_piece);
    }
    this->SetPool(0);
  }

  void Process(size_t location) {
    //Currently just wrapping to use the existing function
    double resources = my_world->PullResources();
    DistribResources(resources);
    // Check reproduction
    if (GetPoints() >= my_config->HOST_REPRO_RES() && repro_syms.size() == 0) {  // if host has more points than required for repro
        // will replicate & mutate a random offset from parent values
        // while resetting resource points for host and symbiont to zero
        emp::Ptr<sggHost> host_baby = emp::NewPtr<sggHost>(random, my_world, my_config, GetIntVal());
        host_baby->mutate();
        //mutate(); //parent mutates and loses current resources, ie new organism but same symbiont
        SetPoints(0);

        //Now check if symbionts get to vertically transmit
        for(size_t j = 0; j< (GetSymbionts()).size(); j++){
          emp::Ptr<Organism> parent = GetSymbionts()[j];
          parent->VerticalTransmission(host_baby);
        }

        //Will need to change this to AddOrgAt and write my own position grabber 
        //when I want ecto-symbionts
        my_world->DoBirth(host_baby, location); //Automatically deals with grid
      }
    if (GetDead()){
        return; //If host is dead, return
      }
    if (HasSym()) { //let each sym do whatever they need to do
        emp::vector<emp::Ptr<Organism>>& syms = GetSymbionts();
        for(size_t j = 0; j < syms.size(); j++){
          emp::Ptr<Organism> curSym = syms[j];
          if (GetDead()){ 
            return; //If previous symbiont killed host, we're done
          }
          if(!curSym->GetDead()){
            curSym->Process(location);
          }
          if(curSym->GetDead()){
            syms.erase(syms.begin() + j); //if the symbiont dies during their process, remove from syms list
            curSym.Delete();
          }
        } //for each sym in syms
      } //if org has syms
  }
};//Host

#endif