








// ************************* try dff cls: >= -2   continue ********************************
// ************************* try dff cls: <= -9   continue ********************************





// The top of every source code file must include this line
#include "sierrachart.h"
#include <vector>            // For using std::vector
#include <sstream>           // For string manipulation
#include <iomanip>


#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

 
#define MAX_VECTOR_SIZE 100000


#define WIN 1 
#define LOSS 2
#define INPROGRESS 3

#define TP_ON_TICKS 16
#define SL_ON_TICKS 5



struct st_BugsDownBar {
    int index;
   
	st_BugsDownBar(int idx) : index(idx) {}
};

struct st_SimTrade {
    int tradeNumber;
	int indexOfPattern ;
	int result ;
   
	st_SimTrade(int tnb , int iop , int rs) : tradeNumber(tnb) , indexOfPattern(iop) , result(rs)  {}
};




bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

int getPriceLevel(SCStudyInterfaceRef sc , int idx )   ;

bool isHoleAtLevel_3(SCStudyInterfaceRef sc , int idx )  ;
bool isHoleAtLevel_2(SCStudyInterfaceRef sc , int idx )  ;
bool isHoleAtLevel_1(SCStudyInterfaceRef sc , int idx )  ;



void findBugsBar(SCStudyInterfaceRef sc , std::vector<st_BugsDownBar>& detectedBugBars )  ;

float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc , int indexOfBugBar)    ;

int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum );

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num )  ;

int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num )  ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );
float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );
float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback );
float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback)  ;

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num );

int getNumBreakPrevHigh(SCStudyInterfaceRef sc , int idx , int lookback )  ;



void simTrade(SCStudyInterfaceRef sc , std::vector<st_BugsDownBar>& detectBugSell , std::vector<st_SimTrade>& detectTrade)  ;


int uniqueNumber = 4221111 ;







SCDLLName("SUA FIND BUGS SELL")



//This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
SCSFExport scsf_FindBugsSell(SCStudyInterfaceRef sc)
{
	
	
	
	SCString msg;
	SCInputRef i_minVol = sc.Input[0];
	
	
	//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;
	
	// ************************************* Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA FIND BUGS SELL";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 0;
	
		
		i_minVol.Name = "Min Volime";
		i_minVol.SetInt(0);
		
		
		
		return;
	}
	
	
	int MIN_START_INDEX = 5;
	int MaxBarLookback = 0;
	
		
	// See if we are capping max bars to check back
	if (MaxBarLookback == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
	int minVol = i_minVol.GetInt();
	
	auto detectedBugsBars = static_cast<std::vector<st_BugsDownBar>*>(sc.GetPersistentPointer(1));
	
	if (!detectedBugsBars) {
        detectedBugsBars = new std::vector<st_BugsDownBar>();
        sc.SetPersistentPointer(1, detectedBugsBars);
    }
	
	
	
	auto filterBugsBars = static_cast<std::vector<st_BugsDownBar>*>(sc.GetPersistentPointer(2));
	
	if (!filterBugsBars) {
        filterBugsBars = new std::vector<st_BugsDownBar>();
        sc.SetPersistentPointer(2, filterBugsBars);
    }
	
	auto sim1s = static_cast<std::vector<st_SimTrade>*>(sc.GetPersistentPointer(3));
	
	if (!sim1s) {
        sim1s = new std::vector<st_SimTrade>();
        sc.SetPersistentPointer(3, sim1s);
    }
	
	
	
	
	
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		
		
	/*	
		
		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodBuyOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodBuyOF->clear();
*/
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	
	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (detectedBugsBars != NULL)
		detectedBugsBars->clear();
	
	if (filterBugsBars != NULL)
		filterBugsBars->clear();
	
	
	if (sim1s != NULL)
		sim1s->clear() ;
	
	
	
	 // ---------------------------- 1. find Bugs Buy OF	 -----------------------
	findBugsBar(sc , *detectedBugsBars)  ;
	
	
	
	
	if(sc.UpdateStartIndex == 0)
	{
		SCString filePath = sc.DataFilesFolder() + "all bugs sell trade.txt";		

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
		sc.GetStudyArrayUsingID(13, 0, vwap);
		
		// for bar duration
		SCFloatArray bar_dur_array;
		sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
		
		SCFloatArray poc_value;
		sc.GetStudyArrayUsingID(3, 41, poc_value);
		
		
		
		
		for (int i = 0; i < detectedBugsBars->size(); i++)
		{
			
			int ix = detectedBugsBars->at(i).index  ;
			
			SCDateTime t1 = sc.BaseDateTimeIn[ix];
			SCString date = sc.DateTimeToString( t1, FLAG_DT_COMPLETE_DATETIME);
			
			
			float howMuchMove = findMaxGoDownBeforeBreakHigh(sc,ix) ;
			int hmm = sc.PriceValueToTicks(howMuchMove) ;
			
			int diff_close_prev = sc.PriceValueToTicks(sc.Close[ix]) - sc.PriceValueToTicks(sc.Close[ix-1])  ;
			
			
			int pos1 = count_pos_table( sc , ix , 1 )   ;
			int pos6 = count_pos_table( sc , ix , 6 )   ;
			int pos7 = count_pos_table( sc , ix , 7 )   ;
			int pos25 = count_pos_table( sc , ix , 25 )   ;
			int pos36 = count_pos_table( sc , ix , 36 )   ;
			
			
			float divVol = vol[ix]/vol[ix-1] ;
			
			float poc_price = poc_value[ix];
			int pocLev = sc.PriceValueToTicks( poc_price - sc.Low[ix] ) ;
		
			
			SCDateTime temp = bar_dur_array[ix] ;
			float bar_sec = temp.GetTimeInSeconds() ;
			
			int i_3 = 3  ;     // 7
			float mean3 = meanVol( sc , ix, i_3 ) ; // 3 , 50 , spacial_mean
			float mean_i_3 = meanVol( sc ,ix-i_3 , i_3) ; //  3 , 50, spacial_mean
			float div_mean = mean3 / mean_i_3 ;
		
			int diffVwap = sc.PriceValueToTicks( vwap[ix]-sc.Close[ix] )    ;
			
			int i_vol = 9;	
			float cum_vol_9 = cummulateVol( sc , ix ,i_vol) ;
			float cum_vol_i_9 = cummulateVol( sc , ix-i_vol ,i_vol) ;
			float div_cum_vol_9 = cum_vol_9/cum_vol_i_9 ;
			
			i_vol = 15;	
			float cum_vol_15 = cummulateVol( sc , ix ,i_vol) ;
			float cum_vol_i_15 = cummulateVol( sc , ix-i_vol ,i_vol) ;
			float div_cum_vol_15 = cum_vol_15/cum_vol_i_15 ;
			
		
			int hmnPocAtLow = findHowManyDownCandleWithPocAtLow(sc ,ix , 9 ) ;	
			
			int hmnPocAtHigh12 = findHowManyUpCandleWithPocAtHigh(sc ,ix , 12 ) ;
			
										
			// Write the data to the file
			outputFile << "Trade: " << i+1 ;
			outputFile << ", ix: " << ix ;
			outputFile << " , vol-1: " << vol[ix-1] ;
			outputFile << " , vol: " << vol[ix] ;
			outputFile << " , pocLev: " << pocLev ;
			outputFile << " , dff cls: " << diff_close_prev ;
			outputFile << " , dff vol: " << divVol ;
			//outputFile << " , pos1: " << pos1 ;
			//outputFile << " , pos2: " << pos2 ;
			//outputFile << " , pos7: " << pos7 ;
			outputFile << " , hmm: " << hmm ;
			outputFile << " , time : " << date << "\n";
			
			
			
			if(diffVwap <= -29 && t1.GetHour() >= 19) continue;
			//if(diffVwap >= 14 && t1.GetHour() >= 19) continue;
			
			if(t1.GetHour() <= 0) continue;
			if(bar_sec == 0)  continue;
			if(bar_sec > 3700)  continue;
			if(divVol > 1.5 && vol[ix-1] >= 23)  continue;
			if(divVol >= 26)  continue;
			if(hmnPocAtHigh12 >= 5)  continue;  
			//if(div_mean >= 0.89 && div_mean <= 1.2)  continue; 
			if(diffVwap >= -1 && diffVwap <= 2)  continue;   
			if(diffVwap >= 5 && diffVwap <= 12)  continue;   
			if(diffVwap >= 26)  continue;  
			//if(div_cum_vol_9 >= 0.96 && div_cum_vol_9 <= 1.19)  continue;  
			if(div_cum_vol_15 >= 0.49 && div_cum_vol_15 <= 0.57)  continue; 
			//if(div_cum_vol_15 <= 0.47)  continue;    
			//if(hmnPocAtLow >= 4)  continue;
			if(pos1 == 0)  continue;   
			//if(pos3 == 1 || pos3 == 6 || pos3 == 7)  continue; 
			if(pos6 == 10)  continue;     
            if(pos7 == 14)  continue;   			
			if(pos25 > 82)  continue;   
			if(pos36 <= 71)  continue;
			
			/*if(bar_sec == 0)  continue;
			if(pos3 == 1 || pos3 == 6 || pos3 == 7)  continue;  
			if(div_cum_vol_15 <= 0.47)  continue;    
			*/
			
			//if( !(pos3 == 1 || pos3 == 6 || pos3 == 7) )  continue;  
			
			filterBugsBars->emplace_back(ix);
			
			
			
			
			
			
			
		}
		
		
				 
		// Close the file
		outputFile.close();
		
		
		
		
		// ################################## SIM ###############################################
		simTrade( sc , *filterBugsBars , *sim1s) ;
		// ################################## SIM ###############################################
		
		
	}	
	

	
	
}



void simTrade(SCStudyInterfaceRef sc , std::vector<st_BugsDownBar>& detectBugSell , std::vector<st_SimTrade>& detectTrade)  
{
	int tradeNumber = 0 ;
	float tp = TP_ON_TICKS*sc.TickSize;
	
	
	detectTrade.clear() ;
		
	// loop in good buy OF
	for (int i = 0; i < detectBugSell.size(); i++)
	{
		
			
		int sellIndex = i;		
		int entryIndex = detectBugSell[i].index ;
		float entryPrice = sc.Low[detectBugSell[i].index] ;
		//float stopLoss = sc.High[detectBugSell[i].endBarIndex] - SL_ON_TICKS*sc.TickSize ;
		float stopLoss = sc.High[detectBugSell[i].index] + 1*sc.TickSize ;  //-1
		tp = entryPrice - TP_ON_TICKS*sc.TickSize  ;
		
		int indexOfPattern ;
		int result ;
		
		// loop from entry bar to end
		for(int j = entryIndex+1 ; j < sc.ArraySize-1; j++)
		{
			// if break high win
			if(sc.Low[j] <= tp)
			{
				// win
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = TP_ON_TICKS ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
			    
				break;				
												
				
			}// if breaK High loss
			else if( sc.High[j] >= stopLoss)
			{
				// loss
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = sc.PriceValueToTicks(entryPrice - stopLoss) ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
				break;		
			}
		}
		
		
	}
	
	
	
	
	SCString filePath = sc.DataFilesFolder() + "Sim Bug Sell Trade.txt";   		

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
	
	
		// it is vol/sec
	SCFloatArray vol_per_sec;
	sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
	
		
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	
	// Get the study ID3 and SG1 (=0)    | it is delta
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);
	
	// Get delta chg
	SCFloatArray delta_chg;
	sc.GetStudyArrayUsingID(3, 2, delta_chg);
	
	// Get max delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);       
	
	// Get min delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	
	// Get min delta
	SCFloatArray percent_delta;
	sc.GetStudyArrayUsingID(3, 10, percent_delta);
	
	// for bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
	
	// for good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);

	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(13, 0, vwap);	  //12
	
	
		

    int totalProfit = 0 ;
	int numberWin = 0;
	int numberLoss = 0;
	
	for(int i = 0 ; i < detectTrade.size() ; i++)
	{
				
		int sellIndex = detectBugSell[detectTrade[i].indexOfPattern].index ;
		int rs = detectTrade[i].result ;
		
		SCDateTime t1 = sc.BaseDateTimeIn[sellIndex];   //detectDBwithGoodBuyOF[i].indexOfGoodOfEndBar
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
		
		float hmm = findMaxGoDownBeforeBreakHigh( sc , sellIndex  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		
		SCDateTime temp = bar_dur_array[sellIndex] ;
		float bar_sec = temp.GetTimeInSeconds() ;
		
				
		int num = 10;              //20
		int cnt_good_sell = 0;
		for(int i = sellIndex-1 ; i >= sellIndex-num ; i--)
		{
			if(goodSellArray[i] == 1) cnt_good_sell++;
		}
		
		int countPos7 = count_pos_table( sc , sellIndex , 7 ) ;	  //7  12
		int countPos28 = count_pos_table( sc , sellIndex , 36 ) ;	//20  25  24  36
		
		int hmnPocAtLow = findHowManyDownCandleWithPocAtLow(sc ,sellIndex , 9 ) ;	
		
		int hmnPocAtHigh = findHowManyUpCandleWithPocAtHigh(sc ,sellIndex , 12 ) ;
		
		
		int i_3 = 3  ;     // 7
		// for mean 3 /
		float mean3 = meanVol( sc , sellIndex, i_3 ) ; // 3 , 50 , spacial_mean
		
		// for mean 3
		float mean_i_3 = meanVol( sc ,sellIndex-i_3 , i_3) ; //  3 , 50, spacial_mean
		
		float div_mean = mean3 / mean_i_3 ;
		
		//float div_vol_ed_prev_idx = volOfEndBar/volOfEndBar ;
		
		int i_vol = 15;	//15
		float cum_vol = cummulateVol( sc , sellIndex ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , sellIndex-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
		
			
		float vps = vol_per_sec[sellIndex] ;
		
		int num_touch_vwap_last_8 = findHowManyTouchVwap( sc , sellIndex , 6 );   //30  , 8
			
		int num_touch_vwap_last_50 = findHowManyTouchVwap( sc , sellIndex , 28 );   //30  , 8
		
		int brkHigh = getNumBreakPrevHigh( sc , sellIndex , 20 )  ;
		
		
		outputFile << "Trade: " << detectTrade[i].tradeNumber ;
		outputFile << ", ix : " << sellIndex ;
		//outputFile << ", touch vwap8 : " << num_touch_vwap_last_8 ;
		//outputFile << ", touch vwap50 : " << num_touch_vwap_last_50 ;
		//outputFile << ", vps : " << vps ;
		//outputFile << ", vol st : " << volOfStartBar;
		outputFile << ", vol ed-1: " << vol[sellIndex-1] ;
		outputFile << ", vol ed/prev : " << vol[sellIndex]/vol[sellIndex-1] ;  
		outputFile << ", bar sec : " << bar_sec ;
		outputFile << ", cnt pos7 : " << countPos7  ; 
		outputFile << ", cnt pos28 : " << countPos28  ; 
		outputFile << ", cnt GS : " << cnt_good_sell ;
		outputFile << ", hmn poc hgh : " << hmnPocAtHigh  ; 
		outputFile << ", div mean : " << div_mean ;	
		outputFile << " , vwap: " << sc.PriceValueToTicks( vwap[sellIndex]-sc.Close[sellIndex] )  ;
		outputFile << " , div cum vol: " << div_cum_vol  ; 
        outputFile << " , brkHigh: " << brkHigh  ;    		
		outputFile << " , result: " << rs ;
		outputFile << " , best move : " << howMuchMove << "T" ;
		outputFile << " , date: " << date ;
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
	
	outputFile << detectTrade.size() << " \n"  ;
	//outputFile << "Tp: " << 1 + 40*sc.TickSize ; 
	//outputFile << " , Sl: " << 1.2 - 5*sc.TickSize ;
	outputFile << "number Win: " << numberWin << "\n" ;
	outputFile << "number Loss: " << numberLoss << "\n" ;
	outputFile << "Total profit: " << totalProfit << "\n" ;
	
    
	// Close the file
	outputFile.close();	
	
}







int getPriceLevel(SCStudyInterfaceRef sc , int idx )
{
	int closePriceInTick = sc.PriceValueToTicks(sc.Close[idx])  ;
	int highPriceInTick = sc.PriceValueToTicks(sc.High[idx])  ;

	
/*	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;
	
	if(numPriceLevel == 5)  return -1;
	
	 const s_VolumeAtPriceV2 *p_VolumeAtPrice=NULL;
    
	SCString msg;
    
	for (int VAPIndex = 0; VAPIndex < numPriceLevel; VAPIndex++)
	{
		if (!sc.VolumeAtPriceForBars->GetVAPElementAtIndex( idx, VAPIndex, &p_VolumeAtPrice))
			break;
        
		int priceInTick = p_VolumeAtPrice->PriceInTicks  ;
		
		
        
		msg.Format("idx = %d | vapIdx = %d | price = %d", idx , VAPIndex , priceInTick );
        //sc.AddMessageToLog(msg, 0);    
    }
*/	
		
	return highPriceInTick - closePriceInTick   ;

	
}


bool isHoleAtLevel_3(SCStudyInterfaceRef sc , int idx )
{
	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;
	int price0 , price1 , price2  ,price3 ;
	if(numPriceLevel != 4)  return false;
	
	 const s_VolumeAtPriceV2 *p_VolumeAtPrice=NULL;
    
	
	for (int VAPIndex = 0; VAPIndex < numPriceLevel; VAPIndex++)
	{
		if (!sc.VolumeAtPriceForBars->GetVAPElementAtIndex( idx, VAPIndex, &p_VolumeAtPrice))
			break;
        
		int priceInTick = p_VolumeAtPrice->PriceInTicks  ;
		
		if(VAPIndex == 2) 	price2 = priceInTick;
		if(VAPIndex == 3) 	price3 = priceInTick;
			       
		
    }
	
	if(price3-price2 > 1)  return true;
	
	return false;
}


bool isHoleAtLevel_2(SCStudyInterfaceRef sc , int idx )
{
	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;
	int price0 , price1 , price2  ,price3 ;
	if(numPriceLevel != 4)  return false;
	
	 const s_VolumeAtPriceV2 *p_VolumeAtPrice=NULL;
    
	
	for (int VAPIndex = 0; VAPIndex < numPriceLevel; VAPIndex++)
	{
		if (!sc.VolumeAtPriceForBars->GetVAPElementAtIndex( idx, VAPIndex, &p_VolumeAtPrice))
			break;
        
		int priceInTick = p_VolumeAtPrice->PriceInTicks  ;
		
		if(VAPIndex == 2) 	price2 = priceInTick;
		if(VAPIndex == 1) 	price1 = priceInTick;
			       
		
    }
	
	if(price2-price1 > 1)  return true;
	
	return false;
}

bool isHoleAtLevel_1(SCStudyInterfaceRef sc , int idx )
{
	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;
	int price0 , price1 , price2  ,price3 ;
	if(numPriceLevel != 4)  return false;
	
	 const s_VolumeAtPriceV2 *p_VolumeAtPrice=NULL;
    
	
	for (int VAPIndex = 0; VAPIndex < numPriceLevel; VAPIndex++)
	{
		if (!sc.VolumeAtPriceForBars->GetVAPElementAtIndex( idx, VAPIndex, &p_VolumeAtPrice))
			break;
        
		int priceInTick = p_VolumeAtPrice->PriceInTicks  ;
		
		if(VAPIndex == 0) 	price0 = priceInTick;
		if(VAPIndex == 1) 	price1 = priceInTick;
			       
		
    }
	
	if(price1-price0 > 1)  return true;
	
	return false;
}


void findBugsBar(SCStudyInterfaceRef sc , std::vector<st_BugsDownBar>& detectedBugBars ) 
{
	
	for (int i = 0; i < sc.ArraySize - 1; ++i) 
	{
		int lev = getPriceLevel( sc , i )  ;
		
		if(lev == 4)
		{
			SCString msg;
			bool isHole1 = isHoleAtLevel_1(sc,i);
			bool isHole2 = isHoleAtLevel_2(sc,i);
			bool isHole3 = isHoleAtLevel_3(sc,i);
			
			
			if(isHole1 || isHole3 || isHole2)
			//if(isHole1 || isHole2)
			{
				msg.Format("idx = %d", i  );
				//sc.AddMessageToLog(msg, 0);  
				detectedBugBars.emplace_back(i );
			}
		}	
	}
	
	
		
	
}


float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc , int indexOfBugBar) 
{
	
	
	int sl = 	sc.PriceValueToTicks(sc.High[indexOfBugBar]) + 1 ;// SL at 
		
		
	int indexThatBreakHigh = -1;
	
	// find which index break high
	for(int i = indexOfBugBar ; i < sc.ArraySize-1 ; i++)
	{
		if( sc.PriceValueToTicks(sc.High[i]) >= sl )
		{
			indexThatBreakHigh = i;
			break;
		}
	}
	
	if(indexThatBreakHigh == -1)
	{
		indexThatBreakHigh = sc.ArraySize-1;
	}
	
	float maxPriceGo = 100;
	
	for(int i = indexOfBugBar ; i <= indexThatBreakHigh ; i++)
	{
		if(sc.Low[i] < maxPriceGo)
		{
			maxPriceGo = sc.Low[i] ;
		}
	}
	
	return sc.Low[indexOfBugBar]-maxPriceGo  ;
	
}


int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum )
{
	
	// Get the study ID3 and SG1 (=0)    | it is delta
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);
	
	// Get delta chg
	SCFloatArray delta_chg;
	sc.GetStudyArrayUsingID(3, 2, delta_chg);
	
	// Get max delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);       
	
	// Get min delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	
	// Get min delta
	SCFloatArray percent_delta;
	sc.GetStudyArrayUsingID(3, 10, percent_delta);
	
	
	int count_of_positive = 0;
	
	for(int k = idx ; k > idx-lookbackColum ; k--)
	{
		if(delta[k] >= 0 ) count_of_positive++ ;
		if(delta_chg[k] >= 0 ) count_of_positive++ ;
		if(max_delta[k] >= 0 ) count_of_positive++ ;
		if(min_delta[k] >= 0 ) count_of_positive++ ;
		if(percent_delta[k] >= 0 ) count_of_positive++ ;
	}
	
	return count_of_positive;
	
}


int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE ) ) cnt++;
	}
	return cnt;
	
}


int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE ) ) cnt++;
	}
	return cnt;
	
}



float meanVol( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	              
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
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
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
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


int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback)
{
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += vol[i];
	}
	return sum;
	
}


int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num )
{
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(13, 0, vwap);	
		
		
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		int i_vwap = sc.PriceValueToTicks( vwap[i] ) ;
		int i_high = sc.PriceValueToTicks( sc.High[i]  ) ;
		int i_low = sc.PriceValueToTicks( sc.Low[i]  );
		
		if(i_vwap >= i_low && i_vwap <= i_high)   cnt++;
		
		
	}
	return cnt;
	
}


int getNumBreakPrevHigh(SCStudyInterfaceRef sc , int idx , int lookback )
{
	if(lookback < 1)  return -1;
	
	int cnt = 0;
	for(int i = 0 ; i < lookback ; i++)
	{
		int index = idx-i  ;
		if(sc.High[index] >= sc.High[index-1]) cnt++  ;
	}
	
	return cnt;
	
}


bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition)
{
	const s_VolumeAtPriceV2* p_vap = NULL;	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;	
	int vol;	
	int levelOFHighestVol ;	
	int highestVol = 0;	
		
	SCFloatArray poc_value;
	sc.GetStudyArrayUsingID(3, 41, poc_value);
	float poc_price = poc_value[idx];
	int pocLev = sc.PriceValueToTicks( poc_price - sc.Low[idx] ) ;

	/*SCString msg2;
	msg2.Format("------------- index %d is valid start bar | poc level = %d | lowest vol = %f | " , idx , pocLev , sc.Low[idx] );
	sc.AddMessageToLog(msg2,0);	*/
		
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
				
		
		
		//if(levelOFHighestVol <= 1) 
		if(pocLev <= 1)
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
	   		
				
		
		//if(levelOFHighestVol >= 3) return true;
		if(pocLev >= 3) return true;
		else return false;
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATTOP)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
				
		//if(levelOFHighestVol >= 3)
		if(pocLev >= 3)		
		{
			SCString msg;
			msg.Format("------------- index %d is valid start bar | poc level = %d | highest vol = %d | " , idx , levelOFHighestVol , highestVol );
			//sc.AddMessageToLog(msg,0);
			return true;
		}
		else 
		{
			return false;
		}
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
		// check is down candle ?
		if ( (sc.Close[idx] - sc.Low[idx]) > 0)
			return false;
	    
	   	 
		
				
	   
		//if(levelOFHighestVol <= 1) 
		if(pocLev <= 1)	
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
	
	return false;
	
}













