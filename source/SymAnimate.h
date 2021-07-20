// This file contains all operations related to the web interface
#ifndef SYM_ANIMATE_H
#define SYM_ANIMATE_H

#include <iostream>
#include "SymWorld.h"
#include "ConfigSetup.h"
//#include "SymJS.h"
#include "Symbiont.h"
#include "Host.h"
#include "Phage.h"
#include "emp/web/Document.hpp"
#include "emp/web/Canvas.hpp"
#include "emp/web/web.hpp"
#include "emp/config/ArgManager.hpp"
#include "emp/prefab/ConfigPanel.hpp"
#include "emp/web/UrlParams.hpp"
#include "WorldSetup.cc"


namespace UI = emp::web;
SymConfigBase config; // load the default configuration
emp::prefab::ConfigPanel config_panel(config);


class SymAnimate : public UI::Animate {
private:

  UI::Document animation;
  UI::Document settings;
  UI::Document explanation;
  UI::Document learnmore;
  UI::Document buttons;
  UI::Canvas mycanvas;

  const int RECT_WIDTH = 10;
  
  emp::Random random{config.SEED()};
  SymWorld world{random};


  emp::vector<emp::Ptr<Organism>> p;


  int num_mutualistic = 0;
  int num_parasitic = 0;

public:

  SymAnimate() : animation("emp_animate"), settings("emp_settings"), explanation("emp_explanation"), learnmore("emp_learnmore"), buttons("emp_buttons") {

    config.GRID_X(50); 
    config.GRID_Y(50); 
    config.UPDATES(30000);

    //Exclude all the settings that control 
    //things that don't show up in the GUI correctly
    config_panel.ExcludeConfig("SYM_LIMIT"); 
    config_panel.ExcludeConfig("DATA_INT");
    config_panel.ExcludeConfig("POP_SIZE");
    config_panel.ExcludeConfig("LYSIS");
    config_panel.ExcludeConfig("BURST_SIZE");
    config_panel.ExcludeConfig("BURST_TIME");
    config_panel.ExcludeConfig("SYM_LYSIS_RES");
    config_panel.ExcludeConfig("START_MOI");
    config_panel.ExcludeConfig("FILE_PATH");
    config_panel.ExcludeConfig("FILE_NAME");
    config_panel.ExcludeConfig("COMPETITION_MODE");

    

    animation.SetCSS("flex-grow", "1");
    animation.SetCSS("max-width", "500px");
    settings.SetCSS("flex-grow", "1");
    settings.SetCSS("max-width", "600px");
    explanation.SetCSS("flex-grow", "1");
    explanation.SetCSS("max-width", "600px");
    learnmore.SetCSS("flex-grow", "1");
    learnmore.SetCSS("max-width", "600px");
    buttons.SetCSS("flex-grow", "1");
    buttons.SetCSS("max-width", "600px");


    initializeWorld();
    emp::prefab::Card config_panel_ex("INIT_CLOSED");
    settings << config_panel_ex;
    config_panel_ex.AddHeaderContent("<h3>Settings</h3>");
    
    // apply configuration query params and config files to config
    auto specs = emp::ArgManager::make_builtin_specs(&config);
    emp::ArgManager am(emp::web::GetUrlParams(), specs);
    // cfg.Read("config.cfg");
    am.UseCallbacks();
    if (am.HasUnused()) std::exit(EXIT_FAILURE);

    // setup configuration panel
    config_panel.Setup();
    config_panel_ex.AddBodyContent(config_panel.GetConfigPanelDiv());


    // Add explanation for organism color:
    explanation << "<br><br><img style=\"max-width:175px;\" src=\"diagram1.png\"> <br>" <<
      "<img style=\"max-width:600px;\" src = \"gradient1.png\"/> <br>";


    // ----------------------- Add a button that allows for pause and start toggle -----------------------
    buttons << "<br>";
    buttons.AddButton([this](){
      // animate up to the number of updates
      ToggleActive();
      auto but = buttons.Button("toggle");
      if (GetActive()) but.SetLabel("Pause");
      else but.SetLabel("Start");
    }, "Start", "toggle");
    setButtonStyle("toggle");
    buttons.Button("toggle").OnMouseOver([this](){ auto but = buttons.Button("toggle"); but.SetCSS("background-color", "grey"); but.SetCSS("cursor", "pointer"); });
    buttons.Button("toggle").OnMouseOut([this](){ auto but = buttons.Button("toggle"); but.SetCSS("background-color", "#D3D3D3"); });

    // ----------------------- Add a reset button to reset the animation/world -----------------------
    /* Note: Must first run world.Reset(), because Inject checks for valid position.
      If a position is occupied, new org is deleted and your world isn't reset.
      Also, canvas must be redrawn to let users see that it is reset */
    buttons.AddButton([this](){
      world.Reset();
      buttons.Text("update").Redraw();
      initializeWorld();
      p = world.getPop();

      if (GetActive()) { // If animation is running, stop animation and adjust button label
        ToggleActive();
      }
      auto but = buttons.Button("toggle");
      but.SetLabel("Start");

      // redraw petri dish
      mycanvas.SetWidth(RECT_WIDTH*config.GRID_X());
      mycanvas.SetHeight(RECT_WIDTH*config.GRID_Y());
      drawPetriDish(mycanvas);
      ToggleActive();//turn on quick to update the grid if the size changed
      ToggleActive();//turn off again
    }, "Reset", "reset");
    setButtonStyle("reset");
    buttons.Button("reset").OnMouseOver([this](){ auto but = buttons.Button("reset"); but.SetCSS("background-color", "grey"); but.SetCSS("cursor", "pointer"); });
    buttons.Button("reset").OnMouseOut([this](){ auto but = buttons.Button("reset"); but.SetCSS("background-color", "#D3D3D3"); });

    // ----------------------- Keep track of number of updates -----------------------
    buttons << "<br>";
    buttons << UI::Text("update") << "Update = " << UI::Live( [this](){ return world.GetUpdate(); } ) << "  ";
    buttons << UI::Text("mut") << "Mutualistic = " << UI::Live( [this](){ return num_mutualistic; } ) << "  ";
    buttons << UI::Text("par") << " Parasitic = " << UI::Live( [this](){ return num_parasitic; } );
    buttons << "<br>";

    // Add a canvas for petri dish and draw the initial petri dish
    mycanvas = animation.AddCanvas(RECT_WIDTH*config.GRID_X(), RECT_WIDTH*config.GRID_Y(), "can");
    targets.push_back(mycanvas);
    drawPetriDish(mycanvas);
    animation << "<br>";

    learnmore << "If you'd like to learn more, please see the publication <a href=\"https://www.mitpressjournals.org/doi/abs/10.1162/artl_a_00273\">Spatial Structure Can Decrease Symbiotic Cooperation</a>.";

  }

  void initializeWorld(){
     // Reset the seed and the random machine of world to ensure consistent result (??)
    random.ResetSeed(config.SEED());
    world.SetRandom(random);

    worldSetup(&world, &config);
    
    p = world.getPop();

  }

  void setButtonStyle(std::string but_id){
    auto but = buttons.Button(but_id);
    but.SetCSS("background-color", "#D3D3D3");
    but.SetCSS("border-radius", "4px");
    but.SetCSS("margin-left", "5px");
  }

  // now draw a virtual petri dish with coordinate offset from the left frame
  void drawPetriDish(UI::Canvas & can){
        int i = 0;
        num_mutualistic = 0;
        num_parasitic = 0;
        //bool temp_passed = true;
        for (int x = 0; x < config.GRID_X(); x++){
            for (int y = 0; y < config.GRID_Y(); y++){
                emp::vector<emp::Ptr<Organism>>& syms = p[i]->GetSymbionts(); // retrieve all syms for this host (assume only 1 sym for each host)
                // color setting for host and symbiont

                std::string color_host = matchColor(p[i]->GetIntVal());
                
                
                
                // Draw host rect and symbiont dot
                can.Rect(x * RECT_WIDTH, y * RECT_WIDTH, RECT_WIDTH, RECT_WIDTH, color_host, "black");
                int radius = RECT_WIDTH / 4;
                if(syms.size() == 1) {
                  std::string color_sym = matchColor(syms[0]->GetIntVal());
                  // while drawing, test whether every organism is mutualistic
                  if (syms[0]->GetIntVal() <= 0) num_parasitic++;
                  else num_mutualistic++;
                  can.Circle(x * RECT_WIDTH + RECT_WIDTH/2, y * RECT_WIDTH + RECT_WIDTH/2, radius, color_sym, "black");
                }
                i++;
                
            }
        }
        //passed = temp_passed; // update passed
  }

  // match the interaction value to colors, assuming that -1.0 <= intVal <= 1.0.
  // The antogonistic have light colors, and the cooperative have dark, brownish colors.
  std::string matchColor(double intVal){
    if ((-1.0 <= intVal) && (intVal < -0.9)) return "#EFFDF0";
    else if ((-0.9 <= intVal) && (intVal < -0.8)) return "#D4FFDD";
    else if ((-0.8 <= intVal) && (intVal < -0.7)) return "#BBFFDB";
    else if ((-0.7 <= intVal) && (intVal < -0.6)) return "#B2FCE3";
    else if ((-0.6 <= intVal) && (intVal < -0.5)) return "#96FFF7";
    else if ((-0.5 <= intVal) && (intVal < -0.4)) return "#86E9FE";
    else if ((-0.4 <= intVal) && (intVal < -0.3)) return "#6FC4FE";
    else if ((-0.3 <= intVal) && (intVal < -0.2)) return "#5E8EFF";
    else if ((-0.2 <= intVal) && (intVal < -0.1)) return "#4755FF";
    else if ((-0.1 <= intVal) && (intVal < 0.0)) return "#5731FD";
    else if ((0.0 <= intVal) && (intVal < 0.1)) return "#7B1DFF";
    else if ((0.1 <= intVal) && (intVal < 0.2)) return "#AB08FF";
    else if ((0.2 <= intVal) && (intVal < 0.3)) return "#E401E7";
    else if ((0.3 <= intVal) && (intVal < 0.4)) return "#D506AD";
    else if ((0.4 <= intVal) && (intVal < 0.5)) return "#CD0778";
    else if ((0.5 <= intVal) && (intVal < 0.6)) return "#B50142";
    else if ((0.6 <= intVal) && (intVal < 0.7)) return "#A7000F";
    else if ((0.7 <= intVal) && (intVal < 0.8)) return "#891901";
    else if ((0.8 <= intVal) && (intVal < 0.9)) return "#7D3002";
    else return "#673F03";
  }



  void DoFrame() {

    if (world.GetUpdate() == config.UPDATES() && GetActive()) {
        ToggleActive();
    } else {
      mycanvas = animation.Canvas("can"); // get canvas by id
      mycanvas.Clear();

      // Update world and draw the new petri dish
      world.Update();
      p = world.getPop();
      drawPetriDish(mycanvas);
      buttons.Text("update").Redraw();
      buttons.Text("mut").Redraw();
      buttons.Text("par").Redraw();
    }
  }


};

#endif
