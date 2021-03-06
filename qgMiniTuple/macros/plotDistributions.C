#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "TH1D.h"
#include "TH2D.h"
#include "THStack.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TVector.h"
#include "binClass.h"
#include "binningConfigurations.h"
#include "treeLooper.h"
#include "../localQGLikelihoodCalculator/localQGLikelihoodCalculator.h"
#include "../localQGLikelihoodCalculator/localQGLikelihoodCalculator.cc"

int main(int argc, char**argv){
  bool overlay 			= true;
  bool norm 			= true;
  bool plot2D			= false;
  bool useDecorrelation		= false;

  // Define binning for plots
  //binClass bins = getSmallEtaBinning();
  binClass bins = getTestBinning();
  //binClass bins = getNoBinning();
  std::vector<TString> types{"quark","gluon"};

  // For different jet types (if _antib is added bTag is applied)
  //for(TString file : {"QCD_Pt-15to3000_Tune4C_Flat_13TeV_pythia8_PU20"}){
  for(TString file : {"QCD_AllPtBins"}){
    for(TString jetType : {"AK4chs"}){
      std::cout << "Making plots for " << jetType << " in file " << file << "..." << std::endl;
      system("rm -rf plots/distributions/" + file + "/" + jetType);

      treeLooper t(file, jetType);						// Init tree
      bins.setReference("pt",  &t.pt);
      bins.setReference("eta", &t.eta);
      bins.setReference("rho", &t.rho);

      // Init local localQGikelihoodCalculator
      //QGLikelihoodCalculator localQG("../data/pdfQG_" + jetType + "_13TeV_v1.root");
      QGLikelihoodCalculator localQG("../data/pdfQG_" + jetType + "_13TeV_80X.root");

      // Creation of histos
      std::map<TString, TH1D*> plots;
      std::map<TString, TH2D*> plots2D;
      for(TString binName : bins.getAllBinNames()){
//        for(TString type : {"quark","gluon","bquark","cquark","pu","undefined"}){
//        for(TString type : {"gluon","quark","quarkFromW","quarkFromT","quarkFromH","quarkFromZ","bQuark","bQuarkFromW","bQuarkFromT","bQuarkFromH","bQuarkFromZ"}){
          for(TString type : types ) {
          TString histName = "_" + type + "_" + binName;
          plots["axis2"  + histName] 	= new TH1D("axis2" + histName, "axis2" + histName, 100, 1, 9);
          plots["ptD"    + histName]	= new TH1D("ptD"   + histName, "ptD"   + histName, 100, 0, 1);
          plots["mult"   + histName]	= new TH1D("mult"  + histName, "mult"  + histName, 100, 0.5, 100.5);
          for(TString var : {"qg","axis2","ptD","mult"}){
            plots[var + "_l" + histName] = new TH1D(var + "_l" + histName, var + "_l" + histName, 25, -0.0001, 1.0001);
          }
          if(plot2D){
            for(TString var2D : {"qg-axis2","qg-ptD","qg-mult","axis2-ptD","axis2-mult","ptD-mult"}){
              plots2D[var2D + histName] = new TH2D(var2D + histName, var2D + histName, 100, -0.0001, 1.0001, 100, -0.0001,1.001);
            }
          }
        }
      }

      // Fill histos
      TString binName;
      while(t.next()){
        if(!bins.getBinName(binName)) 	continue;										// Find bin and return false if outside ranges 
        if(t.jetIdLevel < 3) 		continue;										// Select tight jets
        if(!t.matchedJet) 		continue; 										// Only matched jets
        if(t.bTag) 			continue;										// Anti-b tagging
        if(t.mult < 3) 			continue; 										// Need at least three particles in the jet

        TString type;
/*
        if(t.nGenJetsInCone < 1)		type = "pu";
        else if(!t.matchedJet)			type = "undefined";
        else if(!t.balanced)			type = "undefined";
        else if(t.nGenJetsInCone > 1)		type = "undefined";
        else if(t.nJetsForGenParticle != 1)	type = "undefined";
        else if(t.nGenJetsForGenParticle != 1)	type = "undefined";
        else if(t.partonId == 21) 		type = "gluon";
        else if(fabs(t.partonId) < 4) 		type = "quark";
        else if(fabs(t.partonId) == 4) 		type = "cquark";
        else if(fabs(t.partonId) == 5) 		type = "bquark";
        else 					type = "undefined";
*/
        if(t.nGenJetsInCone < 1)		continue;
//        else if(!t.balanced)			continue;
        else if(t.nGenJetsInCone > 1)		continue;
        else if(t.nJetsForGenParticle != 1)	continue;
        else if(t.nGenJetsForGenParticle != 1)	continue;
        else if(t.partonId == 21) 		type = "gluon";
        else if(fabs(t.partonId) < 4){
          if(fabs(t.motherId) == 24) 		type = "quarkFromW";
          else if(fabs(t.motherId) == 6) 	type = "quarkFromT";
          else if(fabs(t.motherId) == 25) 	type = "quarkFromH";
          else if(fabs(t.motherId) == 23) 	type = "quarkFromZ";
          else 					type = "quark";
        } else if(fabs(t.partonId) == 5){
          if(fabs(t.motherId) == 24) 		type = "bQuarkFromW";
          else if(fabs(t.motherId) == 6) 	type = "bQuarkFromT";
          else if(fabs(t.motherId) == 25) 	type = "bQuarkFromH";
          else if(fabs(t.motherId) == 23) 	type = "bQuarkFromZ";
          else 					type = "bQuark";
        } else					continue;

        float qg_l      = localQG.computeQGLikelihood(t.pt, t.eta, t.rho, {(float) t.mult, t.ptD, t.axis2});
        float axis2_l   = localQG.computeQGLikelihood(t.pt, t.eta, t.rho, {-1, -1, t.axis2});
        float ptD_l     = localQG.computeQGLikelihood(t.pt, t.eta, t.rho, {-1, t.ptD});
        float mult_l    = localQG.computeQGLikelihood(t.pt, t.eta, t.rho, {(float) t.mult});

        TString histName = "_" + type + "_" + binName;

	if (std::find(types.begin(),types.end(),type) == types.end()) continue; // make sure type is in types

        plots["axis2"   + histName]->Fill(t.axis2, 	t.weight);
        plots["ptD"     + histName]->Fill(t.ptD, 	t.weight);
        plots["mult"    + histName]->Fill(t.mult, 	t.weight);
        plots["qg_l"    + histName]->Fill(qg_l,		t.weight);
        plots["axis2_l" + histName]->Fill(axis2_l,	t.weight);
        plots["ptD_l"   + histName]->Fill(ptD_l,	t.weight);
        plots["mult_l"  + histName]->Fill(mult_l,	t.weight);

        if(plot2D){
          plots2D["qg-axis2"   + histName]->Fill(qg_l, 		axis2_l, t.weight);
          plots2D["qg-ptD"     + histName]->Fill(qg_l, 		ptD_l, 	 t.weight);
          plots2D["qg-mult"    + histName]->Fill(qg_l, 		mult_l,  t.weight);
          plots2D["axis2-ptD"  + histName]->Fill(axis2_l, 	ptD_l, 	 t.weight);
          plots2D["axis2-mult" + histName]->Fill(axis2_l, 	mult_l,  t.weight);
          plots2D["ptD-mult"   + histName]->Fill(ptD_l, 	mult_l,  t.weight);
        }
      }
      if(norm) for(auto& plot : plots) plot.second->Scale(1./plot.second->Integral(0, plot.second->GetNbinsX() + 1));

      // Stacking, cosmetics and saving
      for(auto& plot : plots){
        if(!plot.first.Contains("gluon")) continue;
        TCanvas c;
        TLegend l(0.3,0.91,0.7,0.98);
        l.SetNColumns(2);
        l.SetFillColor(kWhite);
        l.SetBorderSize(0);

        TString axisTitle = "";
        if(plot.first.Contains("axis2")) axisTitle = "-log(#sigma_{2})";
        if(plot.first.Contains("ptD")) 	 axisTitle = "p_{T}D";
        if(plot.first.Contains("mult"))  axisTitle = "multiplicity";
        if(plot.first.Contains("qg"))    axisTitle = "quark-gluon";
        if(plot.first.Contains("_l"))	 axisTitle += " likelihood";

        THStack stack(plot.first,";"+axisTitle+";"+(norm?"fraction of ":"")+"jets/bin");
        double maximum = 0;
//        for(TString type : {"gluon","quark","bquark","cquark","pu","undefined"}){
//        for(TString type : {"gluon","quark","quarkFromW","quarkFromT","quarkFromH","quarkFromZ","bQuark","bQuarkFromH","bQuarkFromZ","bQuarkFromW","bQuarkFromT"}){
          for(TString type : types){
          TString histName = plot.first; histName.ReplaceAll("gluon", type);
          if(plots[histName]->GetEntries() == 0) continue;
//          int color = (type == "gluon"? 46 : (type == "quark"? 38 : (type == "bquark"? 32 : (type == "cquark" ? 42 : (type == "pu" ? 39 : 49)))));
          int color = (type == "gluon"? 46 :
                      (type == "quark"? 38 :
                      (type == "quarkFromW"? 2 :
                      (type == "quarkFromT" ? 3 :
                      (type == "quarkFromH" ? 4 :
                      (type == "quarkFromZ" ? 5 :
                      (type == "bQuark" ? 42 :
                      (type == "bQuarkFromH" ? 7 :
                      (type == "bQuarkFromZ" ? 6 :
                      (type == "bQuarkFromT" ? 8 :
                      (type == "bQuarkFromW" ? 28 : 0)))))))))));
//          if(!overlay || type == "gluon" || type == "quark") plots[histName]->SetFillColor(color);
          plots[histName]->SetLineColor(color);
//          plots[histName]->SetLineWidth(type == "quark" || type == "gluon" ? 3 : 2);
          plots[histName]->SetLineWidth(3);
//          if(overlay) plots[histName]->SetFillStyle(type == "quark"? 3004 : 3005);
//          l.AddEntry(plots[histName], (type == "gluon"? "gluon" : (type == "quark"? "uds" : (type == "bquark"? "b" : (type == "cquark"? "c": type)))), "f");
          l.AddEntry(plots[histName], (type == "gluon"? "gluon" :
                                      (type == "quark"? "uds" :
                                      (type == "bQuark"? "b" :
                                      (type == "quarkFromW"? "uds (from W)":
                                      (type == "quarkFromT" ? "uds (from t)" :
                                      (type == "quarkFromH" ? "uds (from H)" :
                                      (type == "quarkFromZ" ? "uds (from Z)" :
                                      (type == "bQuarkFromH"? "b (from H)" :
                                      (type == "bQuarkFromT"? "b (from T)" :
                                      (type == "bQuarkFromZ"? "b (from Z)" :
                                      (type == "bQuarkFromW" ? "b (from W)" : ""))))))))))), "f");
          stack.Add(plots[histName]);
//          if(overlay && (type == "bQuarkFromT" || type == "quarkFromW") && plots[histName]->GetMaximum() > maximum) maximum = plots[histName]->GetMaximum();
          if(overlay && plots[histName]->GetMaximum() > maximum) maximum = plots[histName]->GetMaximum();
        }
        if(!maximum) continue;
        stack.Draw(overlay ? "nostack HIST" : "HIST");
        if(overlay) stack.SetMaximum(maximum*1.1);
        stack.GetXaxis()->CenterTitle();
        stack.GetYaxis()->CenterTitle();
        c.Modified();
        l.Draw();

        bins.printInfoOnPlot(plot.first, jetType);

        TString variable = "";
        for(TString var: {"axis2","ptD","mult","qg"}) if(plot.first.Contains(var)) variable = var;
        if(plot.first.Contains("_l")) variable += "_likelihood";
        if(plot.first.Contains("_cdf")) variable += "_cdf";
        TString pdfDir = "./plots/distributions/" + file + "/" + jetType + "/" + variable + "/";
        TString pdfName = pdfDir + plot.first + ".pdf";
        pdfName.ReplaceAll("_gluon","");
        system("mkdir -p " + pdfDir);
        c.SaveAs(pdfName);
      }


      for(auto& plot : plots2D){
        if(!plot.first.Contains("gluon")) continue;

        TCanvas c;
        TString histName = plot.first; histName.ReplaceAll("gluon", "quark");
        plot.second->SetMarkerColor(kRed);
        plot.second->SetMarkerSize(0.04);
        plot.second->SetMarkerStyle(20);
        plots2D[histName]->SetMarkerColor(kBlue);
        plots2D[histName]->SetMarkerSize(0.04);
        plots2D[histName]->SetMarkerStyle(20);
        if(plot.first.Contains("qg")) 		plot.second->GetXaxis()->SetTitle("quark-gluon likelihood");
        else if(plot.first.Contains("axis2-")) 	plot.second->GetXaxis()->SetTitle("-log(#sigma_{2}) likelihood");
        else if(plot.first.Contains("ptD-")) 	plot.second->GetXaxis()->SetTitle("p_{T}D likelihood");
        if(plot.first.Contains("mult")) 	plot.second->GetYaxis()->SetTitle("multiplicity likelihood");
        else if(plot.first.Contains("-axis2")) 	plot.second->GetYaxis()->SetTitle("-log(#sigma_{2}) likelihood");
        else if(plot.first.Contains("-ptD")) 	plot.second->GetYaxis()->SetTitle("p_{T}D likelihood");
        plot.second->GetXaxis()->CenterTitle();
        plot.second->GetYaxis()->CenterTitle();
        plot.second->SetStats(0);
        plot.second->SetTitle("");
        plot.second->Draw("SCAT");
        plots2D[histName]->Draw("SCAT same");

        bins.printInfoOnPlot(plot.first, jetType);

        TString variables = histName(0, histName.First("_"));
        TString pdfDir = "./plots/distributions_2D/" + file + "/" + jetType + "/" + variables + "/";
        TString pdfName = pdfDir + plot.first + ".pdf";
        pdfName.ReplaceAll("_gluon","");
        system("mkdir -p " + pdfDir);
        c.SaveAs(pdfName);
      }

      for(auto& plot : plots) delete plot.second;
      for(auto& plot : plots2D) delete plot.second;
    }
  }
  return 0;
}
