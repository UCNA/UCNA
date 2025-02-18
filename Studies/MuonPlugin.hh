#ifndef MUONANALYZER_HH
#define MUONANALYZER_HH

#include "OctetAnalyzer.hh"

/// muon data analyzer class
class MuonPlugin: public OctetAnalyzerPlugin {
public:
	/// constructor
	MuonPlugin(OctetAnalyzer* OA);
	
	/// fill from scan data point
	virtual void fillCoreHists(ProcessedDataScanner& PDS, double weight);
	/// calculate muon-spectrum-related info
	virtual void calculateResults();
	/// output plot generation
	virtual void makePlots();
	
	unsigned int nEnergyBins;		///< number of bins for energy histograms
	double energyMax;				///< energy range for energy histograms
	quadHists* qMuonSpectra[2][2];	///< muon-veto event energy for [side][subtracted]
	quadHists* qBackMuons[2][2];	///< back-veto tagged muon spectrum for [side][subtracted]
	fgbgPair* pMuonPos[2];			///< muon event positions
	fgbgPair* pBackMuPos[2];		///< backing veto muon event positions
};

#endif
