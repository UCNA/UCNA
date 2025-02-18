// Quick Script to find the linearized endpoint of Beta Spectra
// Simon Slutsky 06/09/14

// Compile with:
// g++ -o BetaEnergyScaleStudy BetaEnergyScaleStudy.cc `root-config --cflags --glibs`

// Usage: ./BetaEnergyScaleStudy <path-to-file> >> output.txt
// Run on Replayed files

#include <iostream>
#include <TH1.h>
#include <TF1.h>
#include <TFile.h>
#include <TCanvas.h>
#include <math.h>
#include <TStyle.h>
#include <stdio.h>
#include <string>
#include <TVector.h>
#include <TGraph.h>

using namespace std;

double find_lin_endpoint(TH1F* hinput){
  TCanvas * rawcan = new TCanvas();
  hinput->Draw();
  
  // Identify Maximum height
  double max = hinput->GetMaximum();
  int maxbin = hinput->GetMaximumBin();

  // Identify location on spectrum where counts fall to half-max
  int halfwaybin;   
  int NB = hinput->GetNbinsX();
  // look for max diff of 300, smaller might miss the bin
  hinput->GetBinWithContent(max/2., halfwaybin, maxbin, NB, 300); 
  double halfway_energy = hinput->GetBinCenter(halfwaybin);
  double halfway_height = hinput->GetBinContent(halfwaybin);

  // Do a linear fit around the half-max
  TF1 * linfit = new TF1("linfit", "pol1", -100, 3900);
  //  for (int nb = 2; nb < 10; nb++){ //optimized to be around nb = 6
  int nb = 6;
  double fitmin = hinput->GetBinCenter(halfwaybin - nb);
  double fitmax = hinput->GetBinCenter(halfwaybin + nb);
  hinput->Fit("linfit","RQ","",fitmin, fitmax);
  
  // Extract "linearized endpoint"
  double * results = linfit->GetParameters();
  double linendpoint = (-1)*results[0]/results[1];
  
  return linendpoint;
}

// Fit endpoints based on fractional height rather than bin number
double find_lin_endpoint_v2(TH1F* hinput, double lowfrac = 0.2, double highfrac = 0.8){
  TCanvas * rawcan = new TCanvas();
  TH1F * htemp = (TH1F*)hinput->Clone("htemp");
  htemp->Draw();
  
  // Identify Maximum height
  double max = htemp->GetMaximum();
  int maxbin = htemp->GetMaximumBin();
  
  // Identify location on spectrum where counts are at desired fractional height
  int NB = htemp->GetNbinsX();
  int lowbin; 
  int highbin;
  
  htemp->GetBinWithContent(max*lowfrac, lowbin, maxbin, NB, 300);
  htemp->GetBinWithContent(max*highfrac, highbin, maxbin, NB, 300);
  //  cout << "LowFrac: " << lowfrac << ", HighFrac: " << highfrac << endl; 

  double low_energy = htemp->GetBinCenter(lowbin);
  double high_energy = htemp->GetBinCenter(highbin);
  //cout << "LowBin: " << lowbin << ", HighBin: " << highbin << endl; 
  
  // Do a linear fit from high_energy to low_energy
  // (spectrum is decreasing, so ADC counts will be
  // lower when number of events is higher)

  TF1 * linfit = new TF1("linfit", "pol1", -100, 3900);
  htemp->Fit("linfit","RQ","",high_energy, low_energy);
  
  // Extract "linearized endpoint"
  double * results = linfit->GetParameters();
  double linendpoint = (-1)*results[0]/results[1];

  //  cout << "Endpoint: " << linendpoint << endl;
  return linendpoint;

}

TGraph lin_endpoint_v2_study(TH1F * hinput){
  vector <double> lowfitfrac;
  vector <double> endpoint;
  const int N = 20;
  for (int lowi = 0; lowi < N; lowi++){ // Comments toggle varying high or low bound
    //    double lowfitval = 0.2 + lowi*0.025;
    double lowfitval = 0.8 - lowi*0.025;
    lowfitfrac.push_back(lowfitval);
    //endpoint.push_back(find_lin_endpoint_v2(hinput, lowfitval, 0.8));
    endpoint.push_back(find_lin_endpoint_v2(hinput, 0.2, lowfitval));
  }
  TGraph endpoint_graph( N, &(lowfitfrac[0]), &(endpoint[0]) );
  
  return endpoint_graph; 
}

string find_run_num(string filestring){
  int position = filestring.find("spec_");
  string run_num = filestring.substr(position+5, 5);
  return run_num;
}

vector <double> find_all_endpoints(char* filename, int vers = 2){
  string filestring(filename);
  string run_num = find_run_num(filename);

  // ------------------------------------------------
  cout << run_num << "\t"; 
  // ------------------------------------------------
  
  TFile *MyFile = new TFile(filename,"READ");
  TCanvas * c1 = new TCanvas("c1", "c1");
  c1->Divide(2,4);
  gStyle->SetOptFit(1111);
  vector <double> endpoint_vector;
  double endpoint = 0;
  TGraph endpoint_graph;

  const int nSides = 2;
  const int nTubes = 4;
  
  for (int s = 0; s < nSides; s++){
    for (int i = 0; i < nTubes; i++){
      const char * sidechar = s?"W":"E";  // do East (s false) then West (s true)
      TH1F * htemp  = (TH1F*)MyFile->Get( Form("hRawPMTSpectrum_%s%i", sidechar, i) );
      c1->cd( (i+1) + 4*s );
      htemp->Draw();
      if (vers == 1){
	endpoint = find_lin_endpoint(htemp);
      }
      if (vers == 2){
	endpoint = find_lin_endpoint_v2(htemp);
      }
      if (vers == 3){
	endpoint_graph = lin_endpoint_v2_study(htemp);
	TCanvas * c1 = new TCanvas();
	endpoint_graph.Draw("A*");
	c1->SaveAs("EndpointStudy.png");
	c1->SaveAs("EndpointStudy.root");
	return endpoint_vector;
      }
      
      endpoint_vector.push_back(endpoint);
      // ------------------------------------------------
      cout << endpoint_vector[i + 4*s]; //<< "\t";
      // ------------------------------------------------
      if (i + 4*s < nTubes*2 - 1){
	// ------------------------------------------------
	cout << "\t";
	// ------------------------------------------------
      }
    }
  }
  // ------------------------------------------------
  cout << "\n";
  // ------------------------------------------------
  
  c1->SaveAs(  Form("./Figures_Beta/v2/BetaEndpoints_v2_%s.pdf", run_num.c_str()) ); 
  return endpoint_vector;
  
}
 
int main(int argc, char *argv[]){
  if (argc==1){
    cout << "No filename passed. Exiting" << endl;
  }  
  
  char * MyFileName = argv[1];
  vector <double> run_endpoints = find_all_endpoints(MyFileName, 2);

  return 0;
}

