#!/usr/bin/python

import sys
sys.path.append("..")
from ucnacore.LinFitter import *
from ucnacore.PyxUtils import *
from ucnacore.EncalDB import *
from ucnacore.QFile import *
from Asymmetries import runCal
import os
from datetime import datetime

def make_runaxis(rmin,rmax):
	
	tckdist = [5,1]
	if rmax-rmin > 100:
		tckdist = [10,1]
	if rmax-rmin > 500:
		tckdist = [100,10]	
	if rmax-rmin > 1000:
		tckdist = [100,20]
				
	return graph.axis.lin(title="Run Number",min=rmin,max=rmax,
							parter=graph.axis.parter.linear(tickdists=tckdist),
							texter = graph.axis.texter.rational(),
							painter=graph.axis.painter.regular(labeldist=0.1,labeldirection=graph.axis.painter.rotatetext(135)))



class RuncalFile:
	def __init__(self,fname):
		rcals = [runCal(r) for r in QFile(fname).dat.get("runcal",[])]
		self.runcals = dict([(r.run,r) for r in rcals])
		self.runs = self.runcals.keys()
		self.runs.sort()

def plot_run_monitor(rlist,sname,tp="pedestal",outpath=None):

	rlist.sort()
	conn = open_connection()
	
	gdat = []
	for rn in rlist:
		if not rn%10:
			print rn
		cgwgid = getRunMonitorGIDs(conn,rn,sname,tp)
		if not cgwgid:
			print "*** Can't find run monitor for",rn,"***"
			continue	
		cgid,wgid = cgwgid
		cg = getGraph(conn,cgid)
		wg = getGraph(conn,wgid)
		if cg and wg:
			gdat.append([rn,cg[0][2],cg[0][3],wg[0][2],wg[0][3]])
	
	grmon=graph.graphxy(width=25,height=8,
				x=make_runaxis(rlist[0]-1,rlist[-1]+1),
				y=graph.axis.lin(title=sname),
				key = None)
	#grmon.texrunner.set(lfs='foils17pt')

	grmon.plot(graph.data.points(gdat,x=1,y=2,dy=4,title=None),
				[graph.style.errorbar(errorbarattrs=[rgb.blue,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])

	if outpath:
		grmon.writetofile(outpath+"/%s.pdf"%sname)
		
	return grmon
	
def plot_all_pedestals(rmin,rmax):

	rlist = getRunType(open_connection(),"Asymmetry",rmin,rmax)
	
	outpath = os.environ["UCNA_ANA_PLOTS"]+"/test/Pedestals/%i-%i/"%(rlist[0],rlist[-1])
	os.system("mkdir -p %s"%outpath)
	
	for s in ['E','W']:
		for t in range(4):
			sname = "ADC%s%iBeta"%(s,t+1)
			grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
		
		sname = "MWPC%sAnode"%s
		grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
				
		for p in ['x','y']:
			for c in range(16):
				sname = "MWPC%s%s%i"%(s,p,c+1)
				grmon = plot_run_monitor(rlist,sname,"pedestal",outpath)
	
			
def plot_trigeff_history(rmin,rmax):
	
	rlist = getRunType(open_connection(),"Asymmetry",rmin,rmax)
	outpath = os.environ["UCNA_ANA_PLOTS"]+"/test/Trigeff/%i-%i/"%(rlist[0],rlist[-1])
	os.system("mkdir -p %s"%outpath)
	conn = open_connection()
	gheight = 5
	goff1 = 0
	c1 = canvas.canvas()        
	glist1 = []
	goff2 = 0
        c2 = canvas.canvas()
        glist2 = []	

	for s in ['E','W']:
		for t in range(4):
			gdat = []
			for rn in rlist:
				tparms = list(getTrigeffParams(conn,rn,s,t))
				tparms.sort()
				try:
					gdat.append([rn,tparms[0][2],tparms[1][2],tparms[3][2],tparms[3][3]])
				except:
					print "***** Missing data for",rn,s,t
					continue
				if not rn%50:
					print rn
				
			
			gthresh=graph.graphxy(width=25,height=gheight,ypos=goff1,
						x=make_runaxis(rlist[0]-1,rlist[-1]+1),
						y=graph.axis.lin(title="Trigger ADC Threshold (%s%i)"%(s,t),min=0,max=100),
						key = None)
			c1.insert(gthresh)
			glist1.append(gthresh)
			goff1 += gheight+1.7
			gthresh.plot(graph.data.points(gdat,x=1,y=2,dy=3,title=None),
						[graph.style.errorbar(errorbarattrs=[rgb.blue,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])
		
			geff=graph.graphxy(width=25,height=gheight,ypos=goff2,
                                                x=make_runaxis(rlist[0]-1,rlist[-1]+1),
                                                y=graph.axis.lin(title="Trigger Efficiency (%s%i)"%(s,t)),
                                                key = None)
			c2.insert(geff)
			glist2.append(geff)
			goff2 += gheight+1.7
                        geff.plot(graph.data.points(gdat,x=1,y=4,dy=5,title=None),
                                                [graph.style.errorbar(errorbarattrs=[rgb.green,]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red,])])
			geff.plot(graph.data.function("y(x)=1",title=None), [graph.style.line(lineattrs=[style.linestyle.dashed,]),])
			
		goff1 += 0.5
		goff2 += 0.5
	
	c1.writetofile(outpath+"/TrigThresh.pdf")
	c2.writetofile(outpath+"/TrigEff.pdf")


def plot_ped_history():

	RF = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalData/CalDump_Beta_20112012.txt")
	
	runcals = [RF.runcals[r] for r in RF.runs]
	pdat = [(r, r.get_tubesparam("PedMean"), r.get_tubesparam("nPedPts"), r.get_tubesparam("PedRMS")) for r in runcals]
			
	timeticks = (7*86400, 1*86400)
			
	myparter = graph.axis.parter.linear(tickdists=timeticks)
	mytexter = timetexter()
	mytexter.dateformat=r"$%m/%d$"
	gheight = 4
	goff = 0

	avgaxis = graph.axis.lin(title="Beta Runs, PMT Ped Avg per 32 Runs (2011/2012)",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)	
	timeaxis = graph.axis.lin(title="Beta Runs, PMT Pedestals (2011/2012)",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)
	c = canvas.canvas()
	c1 = canvas.canvas()
	glist = []
	alist = []
        	
	for s in ['East','West']:
		
		for t in range(4):
			print s,t
			gdat = [(p[0].midTime(),p[1][(s[0],t)],p[2][(s[0],t)],p[3][(s[0],t)],p[0]) for p in pdat if p[2][(s[0],t)] > 10] # Beta runs.
			#gdat = [(p[0].midTime(),p[1][(s[0],t)],p[2][(s[0],t)],p[3][(s[0],t)],p[0]) for p in pdat if p[2][(s[0],t)] > 1] # Source runs.
			#gdat = [(p[0].midTime(),p[1][(s[0],t)],p[2][(s[0],t)],p[3][(s[0],t)],p[0]) for p in pdat] # Bg runs.

			mu,sigma = musigma([g[1] for g in gdat])
			print gdat[0][0], gdat[-1][0], mu, sigma
			
			meanlist = []
			timelist = []
			for j in range(14):
				sum = 0
                        	avg = 0
				tend = 0
				for i in range(32):
					sum = sum + gdat[32*j + i + 1][1]
					avg = sum/32
					tend = gdat[32*j + i + 1][0]
				meanlist.append(avg)
				timelist.append(tend)
	
			yrange = 5
			if sigma > 5:
				yrange = 50
				
			for g in gdat:
				if abs(g[1]-mu) > 5*sigma:
					print "Unusual pedestals ",g[-1].run,g[1],mu
			
			gaxis = timeaxis
			aaxis = avgaxis
			if glist:
				gaxis = graph.axis.linkedaxis(glist[0].axes["x"])
			if alist:
				aaxis = graph.axis.linkedaxis(glist[0].axes["x"])
			grmon=graph.graphxy(width=25,height=gheight,ypos=goff,
				x=gaxis,
				y=graph.axis.lin(title="%s %i"%(s,t+1),min=mu-yrange,max=mu+yrange,parter=graph.axis.parter.linear(tickdists=[yrange,yrange/5])),
				key = None)
			avmon=graph.graphxy(width=25,height=gheight,ypos=goff,
                                x=aaxis,
                                y=graph.axis.lin(title="%s %i"%(s,t+1),min=mu-25,max=mu+30,parter=graph.axis.parter.linear(tickdists=[20,5])),
                                key = None)
			#setTexrunner(grmon)
			c.insert(grmon)
			c1.insert(avmon)
			glist.append(grmon)
			alist.append(avmon)
			goff += gheight+0.1
			
			grmon.plot(graph.data.points(gdat,x=1,y=2,dy=4,title=None),
				[graph.style.errorbar(),graph.style.symbol(symbol.circle,size=0.10)])
			#grmon.plot(graph.data.points(gdatS,x=1,y=2,dy=4,title=None),
			#	[graph.style.errorbar(errorbarattrs=[rgb.red]),graph.style.symbol(symbol.circle,size=0.10,symbolattrs=[rgb.red])])
			avmon.plot(graph.data.values(x=timelist,y=meanlist),
                                [graph.style.symbol(symbol.circle,size=0.15,symbolattrs=[rgb.red,deco.filled])])			

		goff += 0.5
		
	c.writetofile("/media/Data2/Analysis_Output/test/CalPlots/PMTPedestals_Beta_20112012.pdf")
	c1.writetofile("/media/Data2/Analysis_Output/test/CalPlots/PMTPedAvg_Beta_20112012.pdf")

def plot_gms_history():

	RF = RuncalFile(os.environ["UCNA_ANA_PLOTS"]+"/test/CalData/CalDump_Beta_20112012.txt")
	
	runcals = [RF.runcals[r] for r in RF.runs]
	pdat = [(r, r.get_tubesparam("Pulser0"), r.get_tubesparam("BiMean"), r.get_tubesparam("BiRMS"), r.get_tubesparam("nBiPts")) for r in runcals]

	timeticks = (7*86400, 1*86400)
			
	myparter = graph.axis.parter.linear(tickdists=timeticks)
	mytexter = timetexter()
	mytexter.dateformat=r"$%m/%d$"
	gheight = 4
	goff = 0

	timeaxis = graph.axis.lin(title="Beta Runs (2011/2012), Bi207 - Pulser0",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)
	refaxis = graph.axis.lin(title="Beta Runs (2011/2012), Pulser0",parter=myparter,texter=mytexter,min=pdat[0][0].midTime()-86400,max=pdat[-1][0].midTime()+86400)
	c = canvas.canvas()
	c1 = canvas.canvas()
	glist = []
	alist = []
	
	for s in ['East','West']:
	
		for t in range(4):
			print s,t
			k = (s[0],t)
			gdat = [(p[0].midTime(), (p[2][k]-p[1][k]), p[3][k], p[1][k], p[0]) for p in pdat if p[4][k]>4] #beta
			#gdat = [(p[0].midTime(), (p[2][k]-p[1][k]), p[3][k], p[1][k], p[0]) for p in pdat if p[4][k]>1] #bg,source
			mu,sigma = musigma([g[1] for g in gdat])
                        print gdat[0][0], gdat[-1][0], mu, sigma
                        yrange = 500
                        refyrange = 3400

			for g in gdat:
                        	if (abs(g[1]) > 500) or (g[3] == None):
                                	print "Unusual pedestals ",g[-1].run,g[1],mu

                        gaxis = timeaxis
			raxis = refaxis
                        if glist:
                                gaxis = graph.axis.linkedaxis(glist[0].axes["x"])
                        grmon=graph.graphxy(width=25,height=gheight,ypos=goff,
                       		x=gaxis,
                                y=graph.axis.lin(title="%s %i"%(s,t+1),min=-yrange,max=850), #parter=graph.axis.parter.linear(tickdists=[yrange,yrange/5])),
                                key = None)
			if alist:
                                raxis = graph.axis.linkedaxis(alist[0].axes["x"])
                        refgrmon=graph.graphxy(width=25,height=gheight,ypos=goff,
                                x=raxis,
                                y=graph.axis.lin(title="%s %i"%(s,t+1),min=-100,max=refyrange), #parter=graph.axis.parter.linear(tickdists=[yrange,yrange/5])),
                                key = None)
                        #setTexrunner(grmon)
                        c.insert(grmon)
			c1.insert(refgrmon)
                        glist.append(grmon)
			alist.append(refgrmon)
                        goff += gheight+0.1
                        
                        grmon.plot(graph.data.points(gdat,x=1,y=2,dy=3,title=None), [graph.style.symbol(symbol.circle,size=0.10)])
                        grmon.plot(graph.data.function("y(x)=0",title=None), [graph.style.line(lineattrs=[style.linestyle.dashed,]),])
			refgrmon.plot(graph.data.points(gdat,x=1,y=4,dy=3,title=None), [graph.style.symbol(symbol.circle,size=0.10)])
                        refgrmon.plot(graph.data.function("y(x)=0",title=None), [graph.style.line(lineattrs=[style.linestyle.dashed,]),])

		goff += 0.5
		
	c.writetofile("/media/Data2/Analysis_Output/test/CalPlots/PMTGMS_BetaBiPC_20112012.pdf")
	c1.writetofile("/media/Data2/Analysis_Output/test/CalPlots/PMTGMS_BetaPulser_20112012.pdf")



if __name__ == "__main__":
	
	#plot_all_pedestals(16983,19966)
	
	plot_ped_history()
	#plot_gms_history()

	#plot_trigeff_history(16983,19966)
