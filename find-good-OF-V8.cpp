// The top of every source code file must include this line
#include "sierrachart.h"
#include <vector>            // For using std::vector
#include <sstream>           // For string manipulation
#include <iomanip>


#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  
#define MAX_VECTOR_SIZE 100000


#define WIN 1 
#define LOSS 2
#define INPROGRESS 3

int uniqueNumber = 7889 ;

// Structure to represent an good buy OF start bar
struct st_GoodBuyOFStartBar {
    int index;
   
	st_GoodBuyOFStartBar(int idx) : index(idx) {}
};

// Structure to represent an good buy Orderflow
struct st_GoodBuyOF {
    int startBarIndex;
	int endBarIndex;
	int volume_of_start_bar ;
	int delta_of_start_bar ;
	int volume_of_end_bar ;
	int delta_of_end_bar ;
    SCDateTime time_of_start_bar ;
	SCDateTime time_of_end_bar ;
    int lineNumber;
  
	st_GoodBuyOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
	             int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln ) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) {}
	
};

struct st_SimTrade {
    int tradeNumber;
	int indexOfPattern ;
	int result ;
   
	st_SimTrade(int tnb , int iop , int rs) : tradeNumber(tnb) , indexOfPattern(iop) , result(rs)  {}
};




int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , 
						  std::vector<st_GoodBuyOF>& detectGoodBuyOF);						  

//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;

float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

void simTrade(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOF>& detectGoodBuyOF , std::vector<st_SimTrade>& detectTrade) ;


SCDLLName("SUA FIND GOOD BUY OF")

SCSFExport scsf_FindGoodBuyOF(SCStudyInterfaceRef sc)
{
		
	SCString msg;
	SCInputRef i_minVol = sc.Input[0];
	SCInputRef i_m5_chart_number = sc.Input[1];
	
	//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;
	
	// ************************************* Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA FIND GOOD BUY OF";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 0;
		
		sc.Subgraph[0].Name = "Good buy start buy";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_LINE;
		sc.Subgraph[0].PrimaryColor = RGB (0, 255, 0);
		
		i_minVol.Name = "Min Volime Of good buy orderflow start bar";
		i_minVol.SetInt(0);
		
		// Add an input for the M5 chart number
		
        i_m5_chart_number.Name = "M5 Chart Number";
        i_m5_chart_number.SetInt(7);  // Default to chart number 7
		
		return;
	}
	
	
	// ************************************ Section 2 - Do data processing here
	
	int MIN_START_INDEX = 0;
	int MaxBarLookback = 0;
	
		
	// See if we are capping max bars to check back
	if (MaxBarLookback == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
	// Get the M5 chart number from input
    int M5ChartNumber = i_m5_chart_number.GetInt();
	
		
	
	auto detectedGoodBuyStartBars = static_cast<std::vector<st_GoodBuyOFStartBar>*>(sc.GetPersistentPointer(1));
	
	if (!detectedGoodBuyStartBars) {
        detectedGoodBuyStartBars = new std::vector<st_GoodBuyOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodBuyStartBars);
    }
	
	auto detectGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(2));
	
	if (!detectGoodBuyOF) {
        detectGoodBuyOF = new std::vector<st_GoodBuyOF>();
        sc.SetPersistentPointer(2, detectGoodBuyOF);
    }
	
	auto sim1s = static_cast<std::vector<st_SimTrade>*>(sc.GetPersistentPointer(3));
	
	if (!sim1s) {
        sim1s = new std::vector<st_SimTrade>();
        sc.SetPersistentPointer(3, sim1s);
    }
	
	
	
	int minVol = i_minVol.GetInt();
	//int startIndex = sc.UpdateStartIndex;
	//int length = 1;
	
 
	//A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodBuyOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodBuyOF->clear();

			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (detectedGoodBuyStartBars != NULL)
		detectedGoodBuyStartBars->clear();
	
	
	if (detectGoodBuyOF != NULL)
		detectGoodBuyOF->clear();
	
	if (sim1s != NULL)
		sim1s->clear() ;
	
	
	 // 1.Loop through bars to detect good buy start bar pattern
    for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{
		
		// Check if this is a new bar
		/*if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) {
            continue;
        }*/
		
		
		
		bool isItStartBarOFGoodBuyOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM);
		
		
		// if found good buy start bar , then add it to vector
		if(isItStartBarOFGoodBuyOrderflow)
		{
						
			// use emplace_back
			detectedGoodBuyStartBars->emplace_back(i);			
			
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodBuyStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodBuyStartBars->erase(detectedGoodBuyStartBars->begin());
			}
		
		}
		
		// 2. find end bar of good buy OF
		//findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars , *detectGoodBuyOF);
		
	
	}
	
	 // 2. find end bar of good buy OF
	findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars , *detectGoodBuyOF);
	
	
			
	// 3. draw rectangle of good buy OF
	for (int i = 0; i < detectGoodBuyOF->size(); i++)
	{
		float howMuchMove = findMaxGoUpBeforeBreakLow(sc,detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex) ;
	    int hmm = sc.PriceValueToTicks(howMuchMove) ;
	
		//if(hmm < 40)
			//continue;
		
		// **************************** Draw GOOD BUY OF HERE *********************************************
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectGoodBuyOF->at(i).startBarIndex;
		rectangle.EndIndex = detectGoodBuyOF->at(i).endBarIndex;
		rectangle.BeginValue = sc.High[detectGoodBuyOF->at(i).endBarIndex];
		rectangle.EndValue = sc.Low[detectGoodBuyOF->at(i).startBarIndex];
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodBuyOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
						
		SCString msg;
		msg.Format("start bar : %d | end bar : %d \n" , detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex );
		sc.AddMessageToLog(msg,0);
		
		// draw again to chart 3
		rectangle.Clear();
		rectangle.ChartNumber = 3;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectGoodBuyOF->at(i).startBarIndex;
		rectangle.EndIndex = detectGoodBuyOF->at(i).endBarIndex;
		rectangle.BeginValue = sc.High[detectGoodBuyOF->at(i).endBarIndex];
		rectangle.EndValue = sc.Low[detectGoodBuyOF->at(i).startBarIndex];
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodBuyOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
		
	}
	
	
	if(sc.UpdateStartIndex == 0)
	{
		SCString filePath = sc.DataFilesFolder() + "CollectedData.txt";
		
		
   
		

		// Open the file in append mode
		std::ofstream outputFile;
		if (sc.Index == 0)
		{
			// If it's the first bar, overwrite the file
			outputFile.open(filePath.GetChars(), std::ios::out);
		}
		else
		{
			// Otherwise, append to the file
			outputFile.open(filePath.GetChars(), std::ios::app);
		}

		// Check if the file opened successfully
		if (!outputFile.is_open())
		{
			SCString error;
			error.Format("Failed to open file: %s", filePath.GetChars());
			sc.AddMessageToLog(error, 1);
			return;
		}
		
		
		
		// ################## create object st_GoodBuyOF for collect fittered good buy OF
		auto fillterGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(4));	
		if (!fillterGoodBuyOF) 
		{
			fillterGoodBuyOF = new std::vector<st_GoodBuyOF>();
			sc.SetPersistentPointer(4, fillterGoodBuyOF);
		}		
		if (fillterGoodBuyOF != NULL)
			fillterGoodBuyOF->clear() ;
		// ##################################################################################

		

		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			
			SCString date = sc.DateTimeToString( detectGoodBuyOF->at(i).time_of_end_bar, FLAG_DT_COMPLETE_DATETIME);
			float howMuchMove = findMaxGoUpBeforeBreakLow(sc,detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex) ;
			int hmm = sc.PriceValueToTicks(howMuchMove) ;

			
			// Get the study ID3 and SG1 (=0)    | it is delta
			SCFloatArray delta;
			sc.GetStudyArrayUsingID(3, 0, delta);
			
			// it is cum delta low of start bar
			SCFloatArray cum_delta_low;
			sc.GetStudyArrayUsingID(3, 21, cum_delta_low);
			
			// it is totl vol
			SCFloatArray vol;
			sc.GetStudyArrayUsingID(3, 12, vol);
			
			// it is vol/sec
			SCFloatArray vol_per_sec;
			sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
			
			// it is cvd
			SCFloatArray cum_vol_day;
			sc.GetStudyArrayUsingID(3, 39, cum_vol_day);
			
			// it is min_delta
			SCFloatArray min_delta;
			sc.GetStudyArrayUsingID(3, 8, min_delta);
			
			// it is min_delta
			SCFloatArray max_delta;
			sc.GetStudyArrayUsingID(3, 7, max_delta);
			
			// it is totl vwap
			SCFloatArray vwap;
			sc.GetStudyArrayUsingID(10, 0, vwap);
			
			
			
			// for atr
			int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectGoodBuyOF->at(i).endBarIndex);   // (chart number of m5 , sc.Index)
			SCFloatArray ATRArray;
			sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
			float atrValue = ATRArray[RefChartIndex];
			
			// for ema200
			SCFloatArray EMAArray;
			sc.GetStudyArrayFromChartUsingID(7 , 1 , 0, EMAArray);
			float emaValue = EMAArray[RefChartIndex];
		    
			// for mean vol
			float mean5 = meanVol( sc , detectGoodBuyOF->at(i).endBarIndex, 5 ) ; //5
			
			// for mean delata
			float meanDelta5 = meanDelta(  sc , detectGoodBuyOF->at(i).endBarIndex, 5 ) ;    //5,10
			
			// for mean min delta
			float meanMinDelta10 = meanMinDelta( sc , detectGoodBuyOF->at(i).endBarIndex, 10  );
			
			// for mean max delta
			float meanMaxDelta10 = meanMaxDelta( sc , detectGoodBuyOF->at(i).endBarIndex, 10 ) ;
			
		
		
		
		
			// ################################################ Start filter Good Buy OF Here  ############################################
			
		    //if(hmm < 40) 	continue;			
			if(detectGoodBuyOF->at(i).time_of_end_bar.GetHour() > 18) continue;	
			if(detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex < 80 ) continue;		
			if(detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex > 450 ) continue;
			//if( vol[detectGoodBuyOF->at(i).endBarIndex] < 1000 ) continue;				
			//if( vol[detectGoodBuyOF->at(i).endBarIndex] < mean5/2 ) continue;	
			//if( vol[detectGoodBuyOF->at(i).endBarIndex] < mean5*1 ) continue;	
			//if(meanDelta5*3 >  abs( delta[detectGoodBuyOF->at(i).endBarIndex] )  ) continue;
			//if(meanMinDelta10 > -30 ) continue;			
			//if(meanMaxDelta10 > 100 ) //continue;					
			//if(sc.PriceValueToTicks(atrValue) > 1)  continue;			
			if(vol_per_sec[detectGoodBuyOF->at(i).endBarIndex] > 3)  continue;			//1.4
			//if(detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex >= 3 && vol[detectGoodBuyOF->at(i).endBarIndex] <= 1000) //continue;			
			//if(sc.PriceValueToTicks( vwap[detectGoodBuyOF->at(i).endBarIndex]-sc.Close[detectGoodBuyOF->at(i).endBarIndex] ) <= -3)  continue;			
			//if(sc.PriceValueToTicks( vwap[detectGoodBuyOF->at(i).endBarIndex]-sc.Close[detectGoodBuyOF->at(i).endBarIndex] ) >= 3)  continue;
			//if(cum_vol_day[detectGoodBuyOF->at(i).endBarIndex] > 20000)  continue;
			int startIdx = detectGoodBuyOF->at(i).startBarIndex ;
			int endIdx = detectGoodBuyOF->at(i).endBarIndex ;
			int idxOfGoodBuyOF = i ;
			bool haveStartBarIsSameAsBefore = false;
			float lowOfThisEndBar = sc.Low[endIdx] ;
			float volOfThisEndBar = detectGoodBuyOF->at(i).volume_of_end_bar ;
			int sumOFSameStartIndex = 0;
			for(int j = idxOfGoodBuyOF-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
			{
				
				// if find start bar index same as this index before then dont want
				if( (startIdx == detectGoodBuyOF->at(j).startBarIndex)  )
				{
					//haveStartBarIsSameAsBefore = true;
					//break;
					if( lowOfThisEndBar < sc.Low[detectGoodBuyOF->at(j).endBarIndex]  ) continue;  // if low of this end bar below low of previous end bar
					if(volOfThisEndBar < 10*detectGoodBuyOF->at(j).volume_of_end_bar) continue; // if volOfThisEndBar more 10x of previous same start bar
					if( detectGoodBuyOF->at(j).volume_of_end_bar <= 20)  
					{                                                                  // then it not same
						sumOFSameStartIndex++ ;
					}
					//sumOFSameStartIndex++ ;
				}
			}
			//if(haveStartBarIsSameAsBefore == true)  continue;
			if(sumOFSameStartIndex > 0 )  continue;
			int sumOFTouchHighOfStartIndex = 0;
			for(int k = endIdx-1 ; k >= startIdx+1; k--)
			{
				if(sc.Low[k] <= sc.High[startIdx]+sc.TickSize )   // if touch high of start bar index
				{
					sumOFTouchHighOfStartIndex++;
				}
			}
			if(sumOFTouchHighOfStartIndex > 5 )  continue;  //3
			
			// ########################################### End filter Good Buy OF Here  ############################################
			
			
			
			
	
	        // ######################## Add data to fillterGoodBuyOF Object 
			int idx1 = detectGoodBuyOF->at(i).startBarIndex;
			int v1 = detectGoodBuyOF->at(i).volume_of_start_bar;
			int d1 = detectGoodBuyOF->at(i).delta_of_start_bar;
			SCDateTime t1 = detectGoodBuyOF->at(i).time_of_start_bar;			
			int idx2 = detectGoodBuyOF->at(i).endBarIndex ;
			int v2 = detectGoodBuyOF->at(i).volume_of_end_bar;
			int d2 = detectGoodBuyOF->at(i).delta_of_end_bar;
			SCDateTime t2 = detectGoodBuyOF->at(i).time_of_end_bar;	
			int ln = detectGoodBuyOF->at(i).lineNumber ;						
			fillterGoodBuyOF->emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
			
			
		
			// Write the data to the file
			outputFile << "Trade: " << i+1 ;
			outputFile << ", si: " << detectGoodBuyOF->at(i).startBarIndex ;
			outputFile << " , ei: " << detectGoodBuyOF->at(i).endBarIndex ;
			outputFile << " , diff idx: " << detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex ;
			//outputFile << " , vwap: " << sc.PriceValueToTicks( vwap[detectGoodBuyOF->at(i).endBarIndex]-sc.Close[detectGoodBuyOF->at(i).endBarIndex] )  ;
			outputFile << " , vol: " << vol[detectGoodBuyOF->at(i).endBarIndex] ;
			outputFile << " , mean 5 : " << mean5  ;
			outputFile << " , vol/sec: " << vol_per_sec[detectGoodBuyOF->at(i).endBarIndex] ;
			//outputFile << " , cvd: " << cum_vol_day[detectGoodBuyOF->at(i).endBarIndex] ;meanDelta3
			outputFile << " , mean delta 5: " << meanDelta5;
			outputFile << " , delta: " << delta[detectGoodBuyOF->at(i).endBarIndex] ;  
			//outputFile << " , cumdelta1: " << cum_delta_low[detectGoodBuyOF->at(i).startBarIndex] ;
			//outputFile << " , cumdelta2: " << cum_delta_low[detectGoodBuyOF->at(i).endBarIndex] ;
			//outputFile << " , min delta: " << min_delta[detectGoodBuyOF->at(i).endBarIndex] ;
			//outputFile << " , max delta: " << max_delta[detectGoodBuyOF->at(i).endBarIndex] ;
			outputFile << " , atr: " << sc.PriceValueToTicks(atrValue)  ;
			outputFile << " , ema200: " << sc.PriceValueToTicks(sc.Close[detectGoodBuyOF->at(i).endBarIndex] - emaValue)  ;
			//outputFile << " , mean min delta 10 : " << meanMinDelta10  ;
			//outputFile << " , mean max delta 10 : " << meanMaxDelta10  ;
			outputFile << " , max price move: " /*<< std::setprecision(4) */<< hmm ;
			outputFile << " , time : " << date << "\n";
			
		}
		
		//outputFile << " \ntotal trade : " <<  detectGoodBuyOF->size() << "\n" ;
		//outputFile << sc.TickSize;
		outputFile << sc.PriceValueToTicks(sc.High[sc.ArraySize-1] ) ;
		outputFile << "\nSL: 12 for all trade , Trade Direction is Long only , dont look at si ,ei for analsis\n"  ;	
		outputFile << "if max price move less than 12 mean loss"  ;		
		//outputFile << "I want to know which delta,TP,time range to get best profit"  ;	
		outputFile << "\nsim all trade , which tp level to get best profit , for example , if max price move:20 then you check each trade if max price move < 20 mean loss , if max price move >= 20 mean win\n"  ;	
		outputFile << "and sim total profit lost each tp level"  ;
		//outputFile << "can you fillter trade by delta range,vol range,atr range, diff between si and ei range to get better win rate"  ;	
		
		//can you fillter trade by delta , diff between si and ei range to get better win rate


				 
		// Close the file
		outputFile.close();
		
		
		
		
		// ################################## SIM ###############################################
		simTrade( sc , *fillterGoodBuyOF , *sim1s) ;
		// ################################## SIM ###############################################
		
		
	}
	
	
	
	
	
}

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition)
{
	const s_VolumeAtPriceV2* p_vap = NULL;	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;	
	int vol;	
	int levelOFHighestVol ;	
	int highestVol = 0;	
		
	if(condition == CHECK_5LEVEL)
	{
		if(numPriceLevel == 5)
			return true;
		else
			return false;
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM)
	{
		if(numPriceLevel == 5)
		{}
	    else return false;
		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}		
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}			
			
			vol = p_vap->Volume ;		
			
		}
		
		if(levelOFHighestVol <= 1) 
		{
			//SCString msg;
			//msg.Format("------------- index %d is valid start bar | poc level = %d | highest vol = %d | " , idx , levelOFHighestVol , highestVol );
			//sc.AddMessageToLog(msg,0);
			return true;
		}
		else 
		{
			return false;
		}
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE)
	{
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
		// check is up candle ?
		if ( (sc.Close[idx] - sc.Low[idx]) > 0);
			//return true;
	    else
	   	   return false;
	   		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}
			
			vol = p_vap->Volume ;		
			
		}
		
		if(levelOFHighestVol >= 3) return true;
		else return false;
		
	}
	
	return false;
	
}


void findGoodBuyOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , std::vector<st_GoodBuyOF>& detectGoodBuyOF) 
{
    // loop from first index of start bar of good buy OF to last 
	for (const auto& bar : detectedGoodBuyStartBars) {
        int startBarIndex = bar.index;
		// loop from start bar to current bar
        for (int j = startBarIndex + 1; j < sc.ArraySize - 1; ++j) 
		{
			// check if start bar and end bar far apart 1 Tick ?
            if ((sc.Low[j] >= sc.High[startBarIndex] - 0.00011) && (sc.Low[j] <= sc.High[startBarIndex] + 0.00011))
			{
				// check end bar is 5 level , poc on top , up candle
                if (checkValidBar(sc, j, CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE)) 
				{
					// check no candle go below low of start bar
					bool noCandlebreakLow = true;
					for(int k = startBarIndex ; k <= j ; ++k )
					{
						if(sc.Low[k] < sc.Low[startBarIndex])
						{
							noCandlebreakLow = false ;
							break;
						}
					}					
					// draw if no candle break low of start bar
					if(noCandlebreakLow)
					{
						int idx1 = startBarIndex;
						int v1 = sc.Volume[startBarIndex];
						int d1 = 0;
						SCDateTime t1 = sc.BaseDateTimeIn[startBarIndex];
						
						int idx2 = j ;
						int v2 = sc.Volume[j];
						int d2 = 0;
						SCDateTime t2 = sc.BaseDateTimeIn[j];	

                        int ln = uniqueNumber + startBarIndex + j ;						
						
											
						//auto& data = detectedGoodBuyStartBars;
						detectGoodBuyOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
												
						/*
						//bar.lineNumber = uniqueNumber + startBarIndex + j ;	
						
						// **************************** Draw GOOD BUY OF HERE *********************************************
						s_UseTool rectangle;
						rectangle.Clear();
						rectangle.ChartNumber = sc.ChartNumber;
						rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
						rectangle.AddAsUserDrawnDrawing = 1;
						rectangle.BeginIndex = startBarIndex;
						rectangle.EndIndex = j;
						rectangle.BeginValue = sc.High[j];
						rectangle.EndValue = sc.Low[startBarIndex];
						rectangle.Color = RGB(255, 0, 0);
						rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
						rectangle.LineWidth = 1;
						rectangle.TransparencyLevel = 75;
						rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
						rectangle.LineNumber = ln ;
						sc.UseTool(rectangle);
						
						SCString msg;
						msg.Format("start bar : %d | end bar : %d \n" , startBarIndex , j );
						sc.AddMessageToLog(msg,0);
						
						*/
					}
                 
                }
            }
        }
    }
}

float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) 
{
	//float low = sc.Low[indexOfStartBar] - 0.0001 ;     // SL at low
	float low = sc.High[indexOfEndtBar] - 0.0005 ;       // SL at high-5T
	
	int indexThatBreakLow = -1;
	
	// find which index break low
	for(int i = indexOfEndtBar ; i < sc.ArraySize-1 ; i++)
	{
		if( sc.Low[i] <= low )
		{
			indexThatBreakLow = i;
			break;
		}
	}
	
	if(indexThatBreakLow == -1)
	{
		indexThatBreakLow = sc.ArraySize-1;
	}
	
	float maxPriceGo = 0;
	
	for(int i = indexOfEndtBar ; i <= indexThatBreakLow ; i++)
	{
		if(sc.High[i] > maxPriceGo)
		{
			maxPriceGo = sc.High[i] ;
		}
	}
	
	return maxPriceGo-sc.High[indexOfEndtBar]  ;
	
}


int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex)
{
	
	
	return -1 ;
}

float meanVol( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	float sum=0;
	
	for(int i = index-1 ; i >= index - lookback ; i-- )
	{
		sum += vol[i];
	}
	return sum/lookback ;
}

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);	
	float sum=0;
	
	for(int i = index-1 ; i >= index - lookback ; i-- )
	{
		sum += abs( delta[i] );
	}
	return sum/lookback ;
}


float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);	
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += min_delta[i];
	}
	return sum/lookback ;
}

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);	
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += max_delta[i];
	}
	return sum/lookback ;
}

void simTrade(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOF>& detectGoodBuyOF , std::vector<st_SimTrade>& detectTrade)
{
	int tradeNumber = 0 ;
	float tp = 40*sc.TickSize;
	
	
	detectTrade.clear() ;
		
	// loop in good buy OF
	for (int i = 0; i < detectGoodBuyOF.size(); i++)
	{
		int buyIndex = i;
		
		int entryIndex = detectGoodBuyOF[i].endBarIndex ;
		float entryPrice = sc.High[detectGoodBuyOF[i].endBarIndex] ;
		float stopLoss = sc.High[detectGoodBuyOF[i].endBarIndex] - 5*sc.TickSize ;
		tp = 40*sc.TickSize + entryPrice ;
		
		int indexOfPattern ;
		int result ;
		
		// loop from entry bar to end
		for(int j = entryIndex+1 ; j < sc.ArraySize-1; j++)
		{
			// if break high win
			if(sc.High[j] >= tp)
			{
				// win
				tradeNumber++;
				indexOfPattern = buyIndex;
				result = 40 ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
			    
				break;				
												
				
			}// if breaK LOw loss
			else if( sc.Low[j] <= stopLoss)
			{
				// loss
				tradeNumber++;
				indexOfPattern = buyIndex;
				result = -5 ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
				break;		
			}
		}
		
		
	}
	
	
	
	
	SCString filePath = sc.DataFilesFolder() + "SimTrades1.txt";   		

	// Open the file in append mode
	std::ofstream outputFile;
	if (sc.Index == 0)
	{
		// If it's the first bar, overwrite the file
		outputFile.open(filePath.GetChars(), std::ios::out);
	}
	
	// Check if the file opened successfully
	if (!outputFile.is_open())
	{
		SCString error;
		error.Format("Failed to open file: %s", filePath.GetChars());
		sc.AddMessageToLog(error, 1);
		return;
	}

    int totalProfit = 0 ;
	int numberWin = 0;
	int numberLoss = 0;
	
	for(int i = 0 ; i < detectTrade.size() ; i++)
	{
		int startIndex = detectGoodBuyOF[detectTrade[i].indexOfPattern].startBarIndex ;
		int endIndex =  detectGoodBuyOF[detectTrade[i].indexOfPattern].endBarIndex ;
		int volOfStartBar = detectGoodBuyOF[detectTrade[i].indexOfPattern].volume_of_start_bar ;
		int volOfEndBar = detectGoodBuyOF[detectTrade[i].indexOfPattern].volume_of_end_bar ;
		int rs = detectTrade[i].result ;
		
		outputFile << "Trade: " << detectTrade[i].tradeNumber ;
		outputFile << ", start index : " << startIndex ;
		outputFile << ", end index : " << endIndex ;
		outputFile << ", vol start index : " << volOfStartBar;
		outputFile << ", vol end index : " << volOfEndBar ;
		outputFile << " , result: " << rs ;
		outputFile << " \n"  ;
		
		totalProfit += rs;
		
		if(rs > 0)
		{
			numberWin++;
		}
		else
		{
			numberLoss++;
		}
		
	}
	
	outputFile << " \n"  ;
	//outputFile << "Tp: " << 1 + 40*sc.TickSize ; 
	//outputFile << " , Sl: " << 1.2 - 5*sc.TickSize ;
	outputFile << "number Win: " << numberWin << "\n" ;
	outputFile << "number Loss: " << numberLoss << "\n" ;
	outputFile << "Total profit: " << totalProfit << "\n" ;
	
    
	// Close the file
	outputFile.close();	
	
}


