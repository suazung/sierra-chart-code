














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

 
#define MAX_VECTOR_SIZE 600


#define TP_ON_TICKS 16
#define SL_ON_TICKS 5



struct st_BugsUpBar {
    int index;
   
	st_BugsUpBar(int idx) : index(idx) {}
};



bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

int getPriceLevel(SCStudyInterfaceRef sc , int idx )   ;

bool isHoleAtLevel_3(SCStudyInterfaceRef sc , int idx )  ;
bool isHoleAtLevel_2(SCStudyInterfaceRef sc , int idx )  ;
bool isHoleAtLevel_1(SCStudyInterfaceRef sc , int idx )  ;

void findBugsBar(SCStudyInterfaceRef sc , std::vector<st_BugsUpBar>& detectedBugBars )  ;

int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum );

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num )  ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );
float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );
float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback );
float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback)  ;

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num );

int getNumBreakPrevHigh(SCStudyInterfaceRef sc , int idx , int lookback )  ;

int findHowManyTouchBarRange(SCStudyInterfaceRef sc , int idx , int num ) ;

int findHowManyBugBarBefore(SCStudyInterfaceRef sc , int idx , int num ) ;

int getLastDiffBugBarBefore(SCStudyInterfaceRef sc , int idx , int num );

int getLastDiffGoodSellBugBarBefore(SCStudyInterfaceRef sc , int idx , int num )  ;

void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString) ;

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )  ;



int uniqueNumber = 361211 ;

SCDLLName("SUA ALERT BUG BUY")




SCSFExport scsf_AlertBugBuy(SCStudyInterfaceRef sc)
{
	int count = 0;
	SCString msg;
	
	SCInputRef i_tp_in_ticks = sc.Input[count++];
	SCInputRef AlertSound = sc.Input[count++];
	
	int BUY_SIGNAL = 1 ;
	int M1ChartNumber = 7 ;	
	int BUG_BUY_CODE = 5000	;
	
	SCString m1TimeString , currentTimeString;
	
	
	int persistentUniqueValue = sc.GetPersistentInt(5);   // 5 is key
	
	int lastProcessedBarIndex = sc.GetPersistentInt(6);

	
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA ALERT BUGS BUY";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 3;
		
		sc.Subgraph[0].Name = "BUY SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_BAR;
        sc.Subgraph[0].LineWidth = 7;
		sc.Subgraph[0].PrimaryColor = RGB (0, 255, 0);
				
	
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(15);	
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(2);  // Default alert sound number		
		
		sc.AlertOnlyOncePerBar = true;
		sc.ResetAlertOnNewBar = true; 
		
		
		
		return;
	}
	
	
	int tp_in_ticks = i_tp_in_ticks.GetInt();
		
		
	int MIN_START_INDEX = 5;
	int MaxBarLookback = 0;
	
		
	// See if we are capping max bars to check back
	if (MaxBarLookback == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
		
	
	auto detectedBugsBars = static_cast<std::vector<st_BugsUpBar>*>(sc.GetPersistentPointer(1));
	
	if (!detectedBugsBars) {
        detectedBugsBars = new std::vector<st_BugsUpBar>();
        sc.SetPersistentPointer(1, detectedBugsBars);
    }
	
	
	
	auto filterBugsBars = static_cast<std::vector<st_BugsUpBar>*>(sc.GetPersistentPointer(2));
	
	if (!filterBugsBars) {
        filterBugsBars = new std::vector<st_BugsUpBar>();
        sc.SetPersistentPointer(2, filterBugsBars);
    }
	
	
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
				
		lastProcessedBarIndex = -1;	
		sc.SetPersistentInt(6, lastProcessedBarIndex);
		
		persistentUniqueValue = 0;
		sc.SetPersistentInt(5, persistentUniqueValue);	
			
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
	
	
	
	
	
	 // ---------------------------- 1. find Bugs Buy OF	 -----------------------
	findBugsBar(sc , *detectedBugsBars)  ;
	
	
	
	
	
	// ---------------------------- 2. filter Bugs Buy OF	 -----------------------
	
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
	sc.GetStudyArrayUsingID(12, 0, vwap);
	
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
		int DayOfWeek = t1.GetDayOfWeek();
	
		
		int diff_close_prev = sc.PriceValueToTicks(sc.Close[ix]) - sc.PriceValueToTicks(sc.Close[ix-1])  ;
		
		int pos1 = count_pos_table( sc , ix , 1 )   ;
		int pos2 = count_pos_table( sc , ix , 2 )   ;
		int pos3 = count_pos_table( sc , ix , 3 ) ;	  //7
		int pos7 = count_pos_table( sc , ix , 7 )   ;
		int pos25 = count_pos_table( sc , ix , 25 ) ;	  //7
		
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
		
		int diff_bug_bar_10 = getLastDiffBugBarBefore( sc , ix , 10 )  ;
		
		int diff_GS_bug_bar_4  = getLastDiffGoodSellBugBarBefore(sc , ix , 4 )  ;
									
				
		
		if(DayOfWeek == 1 && t1.GetHour() <= 2) continue;  
		//if(t1.GetHour() >= 21 && t1.GetMinute() >= 50 ) continue; 
		if(t1.GetHour() <= 1) continue;
		if(t1.GetHour() >= 23) continue;
		if(bar_sec == 0)  continue;
		if(div_mean >= 0.89 && div_mean <= 1.2)  continue; 
		if(diffVwap >= -3 && diffVwap <= 1)  continue;   
		if(div_cum_vol_9 >= 0.96 && div_cum_vol_9 <= 1.19)  continue;  
		if(div_cum_vol_15 <= 0.47)  continue;    
		if(hmnPocAtLow >= 4)  continue;
		if(pos3 == 1 || pos3 == 6 || pos3 == 7)  continue;  
		if(pos25 >= 64 && pos25 <= 67)  continue;  
		if(diff_bug_bar_10 >= 8 && diff_bug_bar_10 != 10000)  continue;
		if(diff_GS_bug_bar_4 >= -4 && diff_GS_bug_bar_4 <= -2)  continue;    
		
		
		/*if(bar_sec == 0)  continue;
		if(pos3 == 1 || pos3 == 6 || pos3 == 7)  continue;  
		if(div_cum_vol_15 <= 0.47)  continue;    
		*/
		
		//if( !(pos3 == 1 || pos3 == 6 || pos3 == 7) )  continue;  
		
		
		// ########################################### End filter Bug Buy Here  ############################################
		// ########################################### PUT signal in Subgraph   ############################################		
		
		sc.Subgraph[0][ix] = 1;	
			
			
			
		// ######################## Add data to filterBugsBars Object 
		filterBugsBars->emplace_back(ix);
		
		
	}
	
	
	
	 // ************************* ALERT HERE ************************************
	
	int currentIndex = sc.UpdateStartIndex ; 
	//static int lastProcessedBarIndex = -1;
	
	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1 && 
	   sc.GetBarHasClosedStatus(currentIndex-1)==BHCS_BAR_HAS_CLOSED )
	{
		
		
		 if ( currentIndex != lastProcessedBarIndex  ) // Only send message if it's a new bar
		{
			lastProcessedBarIndex = currentIndex; 
			sc.SetPersistentInt(6, lastProcessedBarIndex);	
			
			SCString message , textToTelegram;
		
						
			int idx_of_bug_bar = currentIndex-1;
			
			
			float stopLoss = sc.Low[idx_of_bug_bar] - sc.TickSize;;
							
			
			int slInTick = sc.PriceValueToTicks( sc.High[idx_of_bug_bar] - stopLoss );
			
			int tpInTick = tp_in_ticks ;
			
			
			
			
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(5, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + BUG_BUY_CODE  ;
			message.Format("BUY | index = %d , uniq = %d" , idx_of_bug_bar , uniqNumber   ); // sc.CurrentSystemDateTime
			sc.AddMessageToLog(message, 0);
			
			bool isTimeCorrect = get_M1Time_currentTime_String( sc ,M1ChartNumber , idx_of_bug_bar ,  BUY_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			if ( /*idx_of_end_bar == sc.ArraySize - 1 - 1  &&*/ isTimeCorrect /*&& sc.GetBarHasClosedStatus(idx_of_end_bar)==BHCS_BAR_HAS_CLOSED*/) 
			{
				if (sc.IsFullRecalculation) return;
				
				sendHttpPostMessage( sc, uniqNumber ,  BUY_SIGNAL ,  m1TimeString , currentTimeString) ;
				 // Update the last bar index where the message was sent
				//sc.SetPersistentInt(6, currentIndex - 1);
				
			}			


									
			stopLoss = sc.Low[idx_of_bug_bar] - sc.TickSize ;
			
	
			
			textToTelegram.Format("GU BUY NOW! at %.4f                                        ", sc.High[idx_of_bug_bar] );
			textToTelegram += "\n";
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("SL at %.4f [%dT]                                        " , stopLoss , slInTick );
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("TP at %.4f [%dT]                                        " , sc.High[idx_of_bug_bar] + (tp_in_ticks*sc.TickSize), tpInTick );
			
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , textToTelegram );
			
		}		
		
	}	
	
}








int getPriceLevel(SCStudyInterfaceRef sc , int idx )
{
	int closePriceInTick = sc.PriceValueToTicks(sc.Close[idx])  ;
	int lowPriceInTick = sc.PriceValueToTicks(sc.Low[idx])  ;

	
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
		
	return closePriceInTick - lowPriceInTick  ;

	
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


void findBugsBar(SCStudyInterfaceRef sc , std::vector<st_BugsUpBar>& detectedBugBars ) 
{
	
	for (int i = 0; i < sc.ArraySize - 1; ++i) 
	{
		int lev = getPriceLevel( sc , i )  ;
		
		if(lev == 4)
		{
			SCString msg;
			bool isHole2 = isHoleAtLevel_2(sc,i);
			bool isHole3 = isHoleAtLevel_3(sc,i);
			bool isHole1 = isHoleAtLevel_1(sc,i);
			
			if(isHole3 || isHole2 || isHole1)
			//if(isHole3 || isHole2 )
			{
				msg.Format("idx = %d", i  );
				//sc.AddMessageToLog(msg, 0);  
				detectedBugBars.emplace_back(i );
			}
		}	
	}
	
	
		
	
}

int findHowManyBugBarBefore(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		int lev = getPriceLevel( sc , i )  ;
		
		if(lev == 4)
		{
			SCString msg;
			bool isHole2 = isHoleAtLevel_2(sc,i);
			bool isHole3 = isHoleAtLevel_3(sc,i);
			bool isHole1 = isHoleAtLevel_1(sc,i);
			
			if(isHole3 || isHole2 || isHole1)
			//if(isHole3 || isHole2 )
			{
				cnt++ ;
								
			}
		}	
	}
	return cnt;
	
}

int getLastDiffBugBarBefore(SCStudyInterfaceRef sc , int idx , int num )
{
		
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		int lev = getPriceLevel( sc , i )  ;
		
		if(lev == 4)
		{
			SCString msg;
			bool isHole2 = isHoleAtLevel_2(sc,i);
			bool isHole3 = isHoleAtLevel_3(sc,i);
			bool isHole1 = isHoleAtLevel_1(sc,i);
			
			if(isHole3 || isHole2 || isHole1)
			//if(isHole3 || isHole2 )
			{
				int lowBugBar = sc.PriceValueToTicks(sc.Low[idx]) ;
				int highOfLastBugBarBefore = sc.PriceValueToTicks(sc.High[i]) ;
				return lowBugBar-highOfLastBugBarBefore ;
				
				
			}
		}	
	}
	return 10000;
	
}


int getLastDiffGoodSellBugBarBefore(SCStudyInterfaceRef sc , int idx , int num )
{
		
	// for good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
	            // 10 ,20
	int cnt_good_sell = 0;

	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(goodSellArray[i] == 1) 
		{
			cnt_good_sell++;
			int lowofLastGoodSell = sc.PriceValueToTicks(sc.Low[i]) ;
			int highOfBugBar = sc.PriceValueToTicks(sc.High[idx]) ;
			return lowofLastGoodSell-highOfBugBar ;
			
		}
	}
	return 10000;
	
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
	sc.GetStudyArrayUsingID(12, 0, vwap);	
		
		
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

int findHowManyTouchBarRange(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int barHigh = sc.PriceValueToTicks( sc.High[idx]  ) ;
	int barLow = sc.PriceValueToTicks( sc.Low[idx]  );
		
		
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		int i_high = sc.PriceValueToTicks( sc.High[i]  ) ;
		int i_low = sc.PriceValueToTicks( sc.Low[i]  );
		
		if( ( (i_high >= barLow) && (i_high <= barHigh) ) 
         || ( (i_low <= barHigh) && (i_low >= barLow) )       ) 	 	
		  				
		  {
			  cnt++;
		  }				
		
		
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


void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString)
{
	SCString postData;
	
	postData.Format("{\"uniq\": %d, \"signal\": %d , \"m1time\": \"%s\", \"currenttime\": \"%s\"}" , 
                uniqNumber, signal, m1TimeString.GetChars(), currentTimeString.GetChars());
	const char* serverURL = "http://localhost:8000/simplesignal";
	
	
	 n_ACSIL::s_HTTPHeader HTTPHeader;
	HTTPHeader.Name = "Content-Type";
	HTTPHeader.Value = "application/json";
	if (!sc.MakeHTTPPOSTRequest(serverURL, postData.GetChars(), &HTTPHeader, 1))
	{
		sc.AddMessageToLog("Error making HTTP request.", 0);
	}
	else
	{
		sc.AddMessageToLog("HTTP request sent successfully.", 0);						
	}
	
}



bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )
{
	
	SCDateTimeArray DateTimeArray;
    sc.GetChartDateTimeArray(M1ChartNumber, DateTimeArray); 
	
	if (DateTimeArray.GetArraySize() == 0)  return false; 
	SCGraphData M1GraphData;
    // Get the base graph data from the specified chart
    sc.GetChartBaseData(M1ChartNumber, M1GraphData);
    // Define a reference to the High array
    SCFloatArrayRef HighArrayM1 = M1GraphData[SC_HIGH];
	SCFloatArrayRef LowArrayM1 = M1GraphData[SC_LOW];
	
	
	int RefChartIndex =sc.GetNearestMatchForDateTimeIndex(M1ChartNumber,IndexOfSL);
	
	float low = sc.Low[IndexOfSL] ;
	float high = sc.High[IndexOfSL] ;
	
	SCString message;	
	
	if(signal == 1)
	{
		int lowInTick = sc.PriceValueToTicks(low) ;
		int lowM1InTick = -1 ;
		
		
		for(int i = RefChartIndex ; i >=0 ; i--)
		{
			lowM1InTick = sc.PriceValueToTicks(LowArrayM1[i]) ;
			if( lowInTick ==  lowM1InTick) 
			{
				//uniqNumber += Index;	
							
				
				SCString DateTimeString = "t m1 = " ; 
				DateTimeString += sc.DateTimeToString(DateTimeArray[i],FLAG_DT_COMPLETE_DATETIME);
				
				sc.AddMessageToLog(DateTimeString, 0);
				
				int Year, Month, Day, Hour, Minute, Second;
				
				//SCString M1TimeString , currentTimeString;
				
				SCDateTime m1time = DateTimeArray[i] ;
				m1time.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);
				m1TimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);						
				
				SCDateTime currentTime = sc.CurrentSystemDateTime;
				currentTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);						
				currentTimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);
			
				return true;
				
			}
			
		}
	}
	else if(signal == -1)
	{
		
		int highInTick = sc.PriceValueToTicks(high) ;
		int highM1InTick = -1 ;
		
		for(int i = RefChartIndex ; i >=0 ; i--)
		{
			highM1InTick = sc.PriceValueToTicks(HighArrayM1[i]) ;
			if( highInTick ==  highM1InTick) 
			{
				//uniqNumber += Index;	
									
								
				SCString DateTimeString = "t m1 = " ; 
				DateTimeString += sc.DateTimeToString(DateTimeArray[i],FLAG_DT_COMPLETE_DATETIME);
				
				sc.AddMessageToLog(DateTimeString, 0);
				
				int Year, Month, Day, Hour, Minute, Second;
				
				//SCString M1TimeString , currentTimeString;
				
				SCDateTime m1time = DateTimeArray[i] ;
				m1time.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);
				m1TimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);						
				
				SCDateTime currentTime = sc.CurrentSystemDateTime;
				currentTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);						
				currentTimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);				
				
				return true;
			}
		}
		
		
	}
	
	return false;
	
	
	
}






















