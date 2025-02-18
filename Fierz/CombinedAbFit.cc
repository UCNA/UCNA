// UCNA includes
#include "G4toPMT.hh"
#include "PenelopeToPMT.hh"
#include "CalDBSQL.hh"
#include "FierzFitter.hh"

// ROOT includes
#include <TH1F.h>
#include <TLegend.h>
#include <TF1.h>
#include <TVirtualFitter.h>
#include <TList.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TMatrixD.h>

// C++ includes
#include <iostream>
#include <fstream>
#include <string>

// C includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


// energy range from the 2013 paper
double min_E = 220;
double max_E = 670;

// expected values (based on the energy range) need to be visible to the chi^2 code
double expected[3][3];


#if 1
using namespace std;
int output_histogram(string filename, TH1F* h, double ax, double ay)
{
	ofstream ofs;
	ofs.open(filename.c_str());
	for (int i = 1; i < h->GetNbinsX(); i++)
	{
		double x = ax * h->GetBinCenter(i);
		//double sx = h->GetBinWidth(i);
		double r = ay * h->GetBinContent(i);
		double sr = ay * h->GetBinError(i);
		ofs << x << '\t' << r << '\t' << sr << endl;
	}
	ofs.close();
	return 0;
}
#endif

struct EnergySpectrum {
    int detector;
    int spin;
    TH1F* central;
    TH1F* asymetric;
    TH1F* fierz;
    vector<double> energy;        
    vector<double> values;        
    vector<double> errors;        

    void init() {
        energy = vector<double>();
	    values = vector<double>();
	    errors = vector<double>();
    }
};


// data need to be globals to be visible by func 
vector<double> asymmetry_energy;        
vector<double> asymmetry_values;        
vector<double> asymmetry_errors;        

vector<double> fierzratio_energy;        
vector<double> fierzratio_values;        
vector<double> fierzratio_errors;        

vector<double> fierzratio_energy;        
vector<double> fierzratio_values;        
vector<double> fierzratio_errors;        



void combined_chi2(Int_t & /*nPar*/, Double_t * /*grad*/ , Double_t &fval, Double_t *p, Int_t /*iflag */  )
{
	double chi2 = 0; 
	double chi,	E; 

	int n = asymmetry_energy.size();
	for (int i = 0; i < n; ++i )
	{
		double par[2] = {p[0],p[1]};
		E = asymmetry_energy[i];
		chi = (asymmetry_values[i] - asymmetry_fit_func(&E,par)) / asymmetry_errors[i];
		chi2 += chi*chi; 
	}

	n = fierzratio_energy.size();
	for (int i = 0; i < n; ++i ) { 
		double par[2] = {p[1], expected[0][1]};
		E = fierzratio_energy[i];
		chi = (fierzratio_values[i] - fierzratio_fit_func(&E,par)) / fierzratio_errors[i];
		chi2 += chi*chi; 
	}
	fval = chi2; 
}



//TF1* combined_fit(TH1F* asymmetry, TH1F* fierzratio, double cov[2][2]) 
TF1* combined_fit(TH1F* asymmetry, TH1F* supersum, double cov[2][2]) 
{ 
	// set up best guess
	int nPar = 2;
	double iniParams[2] = { -0.12, 0 };
	const char * iniParamNames[2] = { "A", "b" };

	// create fit function
	TF1 * func = new TF1("func", asymmetry_fit_func, min_E, max_E, nPar);
	func->SetParameters(iniParams);
	for (int i = 0; i < nPar; i++)
		func->SetParName(i, iniParamNames[i]);

	// fill data structure for fit (coordinates + values + errors) 
	std::cout << "Do global fit" << std::endl;
	// fit now all the function together

	// fill data structure for fit (coordinates + values + errors) 
	TAxis *xaxis1  = asymmetry->GetXaxis();
	TAxis *xaxis2  = fierzratio->GetXaxis();

	int nbinX1 = asymmetry->GetNbinsX(); 
	int nbinX2 = fierzratio->GetNbinsX(); 

	// reset data structure
	asymmetry_energy = vector<double>();
	asymmetry_values = vector<double>();
	asymmetry_errors = vector<double>();
	fierzratio_energy = vector<double>();
	fierzratio_values = vector<double>();
	fierzratio_errors = vector<double>();

	for (int ix = 1; ix <= nbinX1; ++ix)
	{
		double E = xaxis1->GetBinCenter(ix);
		if (min_E < E and E < max_E)
		{
			asymmetry_energy.push_back( E );
			asymmetry_values.push_back( asymmetry->GetBinContent(ix) );
			asymmetry_errors.push_back( asymmetry->GetBinError(ix) );
		}
	}

	for (int ix = 1; ix <= nbinX2; ++ix)
	{
		double E = xaxis2->GetBinCenter(ix);
		if (min_E < E and E < max_E)
		{
			fierzratio_energy.push_back( E );
			fierzratio_values.push_back( fierzratio->GetBinContent(ix) );
			fierzratio_errors.push_back( fierzratio->GetBinError(ix) );
		}
	}

	// set up the minuit fitter
	TVirtualFitter::SetDefaultFitter("Minuit");
	TVirtualFitter * minuit = TVirtualFitter::Fitter(0,nPar);
	for (int i = 0; i < nPar; ++i)
		minuit->SetParameter(i, func->GetParName(i), func->GetParameter(i), 1, 0, 0);
	minuit->SetFCN(combined_chi2);
	minuit->SetErrorDef(1);	// 1 for chi^2

	// set print level
	double arglist[100];
	arglist[0] = 0;
	minuit->ExecuteCommand("SET PRINT",arglist,1);

	// minimize
	arglist[0] = 50; // number of function calls
	arglist[1] = 0.1; // tolerance
	minuit->ExecuteCommand("MIGRAD",arglist,nPar);

	// get result
	double minParams[nPar];
	double parErrors[nPar];
	for (int i = 0; i < nPar; ++i) {  
		minParams[i] = minuit->GetParameter(i);
		parErrors[i] = minuit->GetParError(i);
	}
	double chi2, edm, errdef; 
	int nvpar, nparx;
	minuit->GetStats(chi2,edm,errdef,nvpar,nparx);

	func->SetParameters(minParams);
	func->SetParErrors(parErrors);
	func->SetChisquare(chi2);
	int ndf = asymmetry_energy.size() + fierzratio_energy.size()- nvpar;
	func->SetNDF(ndf);
    
	TMatrixD matrix( nPar, nPar, minuit->GetCovarianceMatrix() );
	for (int i = 0; i < nPar; i++)
		for (int j = 0; j < nPar; j++)
			cov[i][j] = minuit->GetCovarianceMatrixElement(i,j);

	cout << endl;
	cout << "    chi^2 = " << chi2 << ", ndf = " << ndf << ", chi^2/ndf = " << chi2/ndf << endl;
	cout << endl;

	return func; 
}




int main(int argc, char *argv[])
{
	TApplication app("Combined Fit", &argc, argv);

	// load the files that contain our histograms
    TFile *asymmetry_data_tfile = new TFile(
        //"/media/hickerson/boson/Data/Plots/"
        "/media/hickerson/boson/Data/OctetAsym_Offic_2010_FINAL/"
        "Range_0-1000/CorrectAsym/CorrectedAsym.root");
	if (asymmetry_data_tfile->IsZombie())
	{
		std::cout << "File not found." << std::endl;
		exit(1);
	}

    TFile *ucna_data_tfile = new TFile(
        //"/media/hickerson/boson/Data/Plots/"
        "/media/hickerson/boson/Data/OctetAsym_Offic_2010_FINAL/"
		"OctetAsym_Offic.root");
	if (ucna_data_tfile->IsZombie())
	{
		std::cout << "File not found." << std::endl;
		exit(1);
	}

    TFile *sm_mc_tfile = new TFile(
        "/home/xuansun/Documents/SimData_Beta/"
        "xuan_analyzed_baseBetas.root");
		//"Fierz/ratio.root");
	if (sm_mc_tfile->IsZombie())
	{
		std::cout << "File not found." << std::endl;
		exit(1);
	}
    sm_mc_tfile->ls();

    /*
    TFile *fierzratio_data_tfile = new TFile(
		"Fierz/ratio.root");
	if (fierzratio_data_tfile->IsZombie())
	{
		std::cout << "File not found." << std::endl;
		exit(1);
	}
    */

	// extract the histograms from the files
    TH1F *asymmetry_histogram = 
            (TH1F*)asymmetry_data_tfile->Get("hAsym_Corrected_C");
    TH1F *supersum_histogram = 
            (TH1F*)ucna_data_tfile->Get("Total_Events_SuperSum");
    //TH1F *fierzratio_histogram = 
    //        (TH1F*)fierzratio_data_tfile->Get("fierz_ratio_histogram");

	double cov[2][2]; 
	double entries = supersum_histogram->GetEffectiveEntries();
	double N = GetEntries(supersum_histogram, min_E, max_E);

	// set all expectation values for this range
	for (int m = 0; m < 3; m++)
		for (int n = 0; n < 3; n++)
			expected[m][n] = evaluate_expected_fierz(m, n, min_E, max_E);
	
	// find the predicted inverse covariance matrix for this range
	double A = -0.12;
	TMatrixD p_cov_inv(2,2);
	p_cov_inv[0][0] = N * 0.25 * expected[2][0];
	p_cov_inv[1][0] = p_cov_inv[0][1] = -N * 0.25 * A * expected[2][1];
	p_cov_inv[1][1] = N * (expected[0][2] - expected[0][1]*expected[0][1]);

	/// find the covariance matrix
	double det = 0;
	double p_var_A = 1 / p_cov_inv[0][0];
	double p_var_b = 1 / p_cov_inv[1][1];
	TMatrixD p_cov = p_cov_inv.Invert(&det);

	/// do the fitting
	//TF1* func = combined_fit(asymmetry_histogram, fierzratio_histogram, cov);
	TF1* func = combined_fit(asymmetry_histogram, supersum_histogram, cov);

	/// output the data info
	cout << " ENERGY RANGE:" << endl;
	cout << "    Energy range is " << min_E << " - " << max_E << " keV" << endl;
	cout << "    Number of counts in full data is " << (int)entries << endl;
	cout << "    Number of counts in energy range is " <<  (int)N << endl;
	cout << endl;

	/// output the details	
	cout << " FIT COVARIANCE MATRIX cov(A,b) =\n";
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			cout << "\t" << cov[i][j];
		}
		cout << "\n";
	}

	double sig_A = sqrt(cov[0][0]);
	double sig_b = sqrt(cov[1][1]);

	cout << endl;
	cout << " PREDICTED COVARIANCE MATRIX cov(A,b) =\n";
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			cout << "\t" << p_cov[i][j];
		}
		cout << "\n";
	}

	double p_sig_A = sqrt(p_cov[0][0]);
	double p_sig_b = sqrt(p_cov[1][1]);

	cout << endl;
	cout << " FOR UNCOMBINED FITS:\n";
	cout << "    Expected statistical error for A in this range is without b is " 
					<< sqrt(p_var_A) << endl;
	cout << "    Expected statistical error for b in this range is without A is "
					<< sqrt(p_var_b) << endl;
	cout << endl;
	cout << " FOR COMBINED FITS:\n";
	cout << "    Expected statistical error for A in this range is " << p_sig_A << endl;
	cout << "    Actual statistical error for A in this range is " << sig_A << endl;
	cout << "    Ratio for A error is " << sig_A / p_sig_A << endl;
	cout << "    Expected statistical error for b in this range is " << p_sig_b << endl;
	cout << "    Actual statistical error for b in this range is " << sig_b << endl;
	cout << "    Ratio for b error is " << sig_b / p_sig_b << endl;
	cout << "    Expected cor(A,b) = " << p_cov[1][0] / p_sig_A / p_sig_b << endl;
	cout << "    Actual cor(A,b) = " << cov[1][0] / sqrt(cov[0][0] * cov[1][1]) << endl;

	/*
	// A fit histogram for output to gnuplot
    TH1F *fierz_fit_histogram = new TH1F(*asymmetry_histogram);
	for (int i = 0; i < fierz_fit_histogram->GetNbinsX(); i++)
		fierz_fit_histogram->SetBinContent(i, fierz_fit->Eval(fierz_fit_histogram->GetBinCenter(i)));

	

	/// output for root
    TString pdf_filename = "/data/kevinh/mc/asymmetry_fierz_term_fit.pdf";
    canvas->SaveAs(pdf_filename);

	/// output for gnuplot
	//output_histogram("/data/kevinh/mc/super-sum-data.dat", super_sum_histogram, 1, 1000);
	//output_histogram("/data/kevinh/mc/super-sum-mc.dat", mc.sm_super_sum_histogram, 1, 1000);
	//output_histogram("/data/kevinh/mc/fierz-ratio.dat", fierz_ratio_histogram, 1, 1);
	//output_histogram("/data/kevinh/mc/fierz-fit.dat", fierz_fit_histogram, 1, 1);

	*/

	// Create a new canvas.
	TCanvas * c1 = new TCanvas("c1","Two HIstogram Fit example",100,10,900,800);
	c1->Divide(2,2);
	gStyle->SetOptFit();
	gStyle->SetStatY(0.6);

	c1->cd(1);
	asymmetry_histogram->Draw();
	func->SetRange(min_E, max_E);
	func->DrawCopy("cont1 same");
	/*
	c1->cd(2);
	asymmetry->Draw("lego");
	func->DrawCopy("surf1 same");
	*/
	c1->cd(3);
	func->SetRange(min_E, max_E);
	///TODO fierzratio_histogram->Draw();
	func->DrawCopy("cont1 same");
	/*
	c1->cd(4);
	fierzratio->Draw("lego");
	gPad->SetLogz();
	func->Draw("surf1 same");
	*/

	app.Run();

	return 0;
}
