// The top of every source code file must include this line
#include "sierrachart.h"


#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 6000

SCDLLName("SUA FIND DT DB")


int uniqueNumber = 3435342 ;
int uniqueNumberForPivot = 84561 ;
int uniqueNumberForPivotGoodBuyOF = 242445 ;
int uniqueNumberForPivotGoodSellOF = 532341 ;

// Structure to represent an PivotHigh
struct st_PivotHigh {
	int index;
	int lineNumber;
   
	st_PivotHigh(int idx , int ln) : index(idx) ,lineNumber(ln) {}
};

// Structure to represent an PivotLow
struct st_PivotLow {
	int index;
	int lineNumber;
   
	st_PivotLow(int idx , int ln) : index(idx) ,lineNumber(ln) {}
};

// Structure to represent an good sell OF start bar
struct st_GoodSellOFStartBar {
	int index;
   
	st_GoodSellOFStartBar(int idx) : index(idx) {}
};

// Structure to represent an good sell Orderflow
struct st_GoodSellOF {
	int startBarIndex;
	int endBarIndex;
	int volume_of_start_bar ;
	int delta_of_start_bar ;
	int volume_of_end_bar ;
	int delta_of_end_bar ;
	SCDateTime time_of_start_bar ;
	SCDateTime time_of_end_bar ;
	int lineNumber;
  
	st_GoodSellOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
				 int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln ) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) {}
	
};

// Structure to represent an good sell OF start bar
struct st_DoubleTopWithGoodOF {
	int lineNumberOf3tzone;
	int lineNumberOfGoodOf;
	int indexOfPivotHigh;
	int indexOfGoodOfStartBar;
	int indexOfGoodOfEndBar;
	float highOfPivotHigh;
	float highOfGoodOF;
	float lowOfGoodOF;
   
	st_DoubleTopWithGoodOF(int i1, int i2 , int i3 , int i4 , int i5 ,
	                       float f1 , float f2 , float f3) 
						   : lineNumberOf3tzone(i1), lineNumberOfGoodOf(i2), indexOfPivotHigh(i3) ,indexOfGoodOfStartBar(i4) , indexOfGoodOfEndBar(i5) ,
 						     highOfPivotHigh(f1) , highOfGoodOF(f2) , lowOfGoodOF(f3) {}
};


struct st_SimTrade {
    int tradeNumber;
	int indexOfPattern ;
	int result ;
   
	st_SimTrade(int tnb , int iop , int rs) : tradeNumber(tnb) , indexOfPattern(iop) , result(rs)  {}
};



bool iSPivotHigh(SCStudyInterfaceRef sc , int index, int pivotLength) ;
bool iSPivotLow(SCStudyInterfaceRef sc , int index, int pivotLength) ;

int isThisStartBarOFGoodSellOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodSellOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , 
						  std::vector<st_GoodSellOF>& detectGoodSellOF);
						  
float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;		

void simTrade(SCStudyInterfaceRef sc , std::vector<st_DoubleTopWithGoodOF>& detectDTwithGoodSellOF , std::vector<st_SimTrade>& detectTrade , int tp_in_ticks) ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );



				  
						  

//This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
SCSFExport scsf_FindDtDb(SCStudyInterfaceRef sc)
{
	
	SCString msg;
	
	SCInputRef i_minBar = sc.Input[0];
	SCInputRef i_pivotLength = sc.Input[1];
	SCInputRef i_tp_in_ticks = sc.Input[2];
	
	
	
	// Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA FIND DT DB";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 0;
		
		i_pivotLength.Name = "number of bar each side | ex. 4 is left 4 bars , right 4 bars";
		i_pivotLength.SetInt(4);
		
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(40);
		
		i_minBar.Name = "min number of bar";
		i_minBar.SetInt(4);
		
		
		
		
		
		return;
	}
	
	
	// Section 2 - Do data processing here
	
	int MaxBarLookback = 0;
	int MIN_START_INDEX = 2;   // need 3 so it is 0 1 2
	
	int minBar = i_minBar.GetInt();
	int pivotLength = i_pivotLength.GetInt();
	int tp_in_ticks = i_tp_in_ticks.GetInt();
	
	auto detectPivotHighs = static_cast<std::vector<st_PivotHigh>*>(sc.GetPersistentPointer(1)); 
	auto detectPivotLows = static_cast<std::vector<st_PivotHigh>*>(sc.GetPersistentPointer(2)); 
	auto detectedGoodSellStartBars = static_cast<std::vector<st_GoodSellOFStartBar>*>(sc.GetPersistentPointer(3));
	auto detectGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(4));
	auto detectDTWithGoodOF = static_cast<std::vector<st_DoubleTopWithGoodOF>*>(sc.GetPersistentPointer(5));
	auto sim1s = static_cast<std::vector<st_SimTrade>*>(sc.GetPersistentPointer(6));
	auto fillterDTwithGoodSellOF = static_cast<std::vector<st_DoubleTopWithGoodOF>*>(sc.GetPersistentPointer(7));
	
	
	if (!detectPivotHighs) {
        detectPivotHighs = new std::vector<st_PivotHigh>();
        sc.SetPersistentPointer(1, detectPivotHighs);
    }
	
	if (!detectPivotLows) {
        detectPivotLows = new std::vector<st_PivotHigh>();
        sc.SetPersistentPointer(2, detectPivotLows);
		
    }	
	
	if (!detectedGoodSellStartBars) {
        detectedGoodSellStartBars = new std::vector<st_GoodSellOFStartBar>();
        sc.SetPersistentPointer(3, detectedGoodSellStartBars);
    }	
	
	if (!detectGoodSellOF) {
        detectGoodSellOF = new std::vector<st_GoodSellOF>();
        sc.SetPersistentPointer(4, detectGoodSellOF);
    }
	
	if (!detectDTWithGoodOF) {
        detectDTWithGoodOF = new std::vector<st_DoubleTopWithGoodOF>();
        sc.SetPersistentPointer(5, detectDTWithGoodOF);
    }
	
	if (!sim1s) {
        sim1s = new std::vector<st_SimTrade>();
        sc.SetPersistentPointer(6, sim1s);
    }
	
	if (!fillterDTwithGoodSellOF) 
	{
		fillterDTwithGoodSellOF = new std::vector<st_DoubleTopWithGoodOF>();
		sc.SetPersistentPointer(7, fillterDTwithGoodSellOF);
	}	
	
	
	
	if(pivotLength > MIN_START_INDEX)
	{
		MIN_START_INDEX = pivotLength*2 ;
	}
	
	if (MaxBarLookback == 0 )
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
	
	//A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		for (int i = 0; i < detectPivotHighs->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectPivotHighs->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectPivotHighs->clear();
		
		for (int i = 0; i < detectPivotLows->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectPivotLows->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectPivotLows->clear();
		
		for (int i = 0; i < detectGoodSellOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodSellOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodSellOF->clear();
		
		for (int i = 0; i < detectDTWithGoodOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDTWithGoodOF->at(i).lineNumberOf3tzone);
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDTWithGoodOF->at(i).lineNumberOfGoodOf);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectDTWithGoodOF->clear();
				
				
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	
	// ################################         MUST clear vector for                ###########################################
	// ################################      1. help speed up 3 times                ###########################################
	// ################################      2. protect it add same data	         ###########################################
	
	if (detectPivotHighs != NULL)
		detectPivotHighs->clear();
	
	if (detectPivotLows != NULL)
		detectPivotLows->clear();
	
	if (detectedGoodSellStartBars != NULL)
		detectedGoodSellStartBars->clear();
	
	if (detectGoodSellOF != NULL)
		detectGoodSellOF->clear();
	
	if (detectDTWithGoodOF != NULL)
		detectDTWithGoodOF->clear();
	
	if (sim1s != NULL)
		sim1s->clear();
	
	if (fillterDTwithGoodSellOF != NULL)
		fillterDTwithGoodSellOF->clear();
	
	
	 // 1.Loop through bars to DT  pattern
    for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{
		
		// Check if this is a new bar
		/*if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) {
            continue;
        }*/
		
		bool checkPivotHigh = iSPivotHigh(sc , i ,pivotLength );
		bool checkPivotLow = iSPivotLow(sc , i ,pivotLength );
		int ln = uniqueNumberForPivot+i ;
		
		// if found pivot high , then add it to vector
		if(checkPivotHigh)
		{						
			// use emplace_back
			detectPivotHighs->emplace_back(i ,ln );			
			
			// Ensure the vector does not exceed the maximum size
			if (detectPivotHighs->size() > MAX_VECTOR_SIZE) {
				detectPivotHighs->erase(detectPivotHighs->begin());
			}
		
		}	
		
		// if found pivot low , then add it to vector
		if(checkPivotLow)
		{						
			// use emplace_back
			detectPivotLows->emplace_back(i ,ln );			
			
			// Ensure the vector does not exceed the maximum size
			if (detectPivotLows->size() > MAX_VECTOR_SIZE) {
				detectPivotLows->erase(detectPivotLows->begin());
			}
		
		}	
		
		//##############################                   ##############################################
		//############################## FIND GOOD SELL Start Bar ##############################################
		//##############################                   ##############################################
		
		bool isItStartBarOFGoodSellOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP);
		
		
		// if found good Sell start bar , then add it to vector
		if(isItStartBarOFGoodSellOrderflow)
		{
						
			// use emplace_back
			detectedGoodSellStartBars->emplace_back(i);			
			
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodSellStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodSellStartBars->erase(detectedGoodSellStartBars->begin());
			}
		
		}
			
	}	
	
	 // 2. find end bar of good sell OF
	findGoodSellOrderFlow(sc , *detectedGoodSellStartBars , *detectGoodSellOF);
	
	
	
	// 3. loop from start of good sell OF
	//for (const auto& goodSellOF : *detectGoodSellOF)
	for (int i = 0 ; i < detectGoodSellOF->size() ; i++)
	{
		int indexOfStartBar = detectGoodSellOF->at(i).startBarIndex;
		float highOfStartBar = sc.High[indexOfStartBar] ;
		// find at left side of Good OF have pivot high or not
		for(int j = detectPivotHighs->size() - 1 ; j >= 0 ; j--)
		{
			SCString msg;
			//msg.Format("j : %d \n" ,j );
			//sc.AddMessageToLog(msg,0);
			int indexOfPivotHigh = detectPivotHighs->at(j).index;
			// if find index of bar of pivot high at leftside
			if( indexOfPivotHigh < indexOfStartBar)
			{
				// check if high not +- more3T
				float highOfPivot = sc.High[detectPivotHighs->at(j).index];
				int p = sc.PriceValueToTicks(highOfPivot) -  sc.PriceValueToTicks(highOfStartBar) ;
				if(abs(p) <= 3 )
				{
					bool noCandleBreakHigh = true;
					float maxValue = max(highOfPivot, highOfStartBar) ;
					// loop from start bar to pivot high ,then check if no candle break high of both
					for(int k = indexOfStartBar-1 ; k >= indexOfPivotHigh ; k--)
					{
						
						if(sc.High[k] > maxValue)
						{
							noCandleBreakHigh = false ;
							break;
						}
					}
					if(noCandleBreakHigh)
					{
												
						int ln1 = detectPivotHighs->at(j).lineNumber;
						int ln2 = detectGoodSellOF->at(i).lineNumber;
						int idxPivotHigh = indexOfPivotHigh ;
						int idxStartBar = indexOfStartBar ;
						int idxOFEndBar = detectGoodSellOF->at(i).endBarIndex;
						float hghOfPivot = highOfPivot ;
						float hghOfGoodOF = sc.High[idxStartBar] ;
						float lowOfGoodOF = sc.Low[idxOFEndBar] ;
							 
						detectDTWithGoodOF->emplace_back(ln1 , ln2 ,idxPivotHigh , idxStartBar , idxOFEndBar , hghOfPivot , hghOfGoodOF , lowOfGoodOF);	
					}
						
				}
			}
			
		}
	}
		
	
	
	
	// for atr
	//int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, sc.ArraySize-1);   // (chart number of m5 , sc.Index)
	//SCFloatArray ATRArray;
	//sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
	//float atrValue = ATRArray[RefChartIndex];
	
/*	for(int i = 0 ; i < detectPivotHighs->size() ; i++)
	{		
		s_UseTool Tool;  
		Tool.Clear(); // reset tool structure for our next use
		Tool.ChartNumber = sc.ChartNumber;
		Tool.DrawingType = DRAWING_TEXT;
		Tool.LineNumber =  detectPivotHighs->at(i).lineNumber ;
		Tool.BeginIndex = detectPivotHighs->at(i).index;
		Tool.BeginValue = sc.High[detectPivotHighs->at(i).index] + 0.0003;
		Tool.Color = RGB(255,255,0);  // Yellow
		//Tool.Text.Format("ATR Value: %f",atrValue );
		Tool.Text = "AHH";
		Tool.FontSize = 16;
		Tool.AddMethod = UTAM_ADD_OR_ADJUST;
		
		sc.UseTool(Tool);		
	}
*/	
	for(int i = 0 ; i < detectPivotLows->size() ; i++)
	{		
		s_UseTool Tool;  
		Tool.Clear(); // reset tool structure for our next use
		Tool.ChartNumber = sc.ChartNumber;
		Tool.DrawingType = DRAWING_TEXT;
		Tool.LineNumber =  detectPivotLows->at(i).lineNumber ;
		Tool.BeginIndex = detectPivotLows->at(i).index;
		Tool.BeginValue = sc.Low[detectPivotLows->at(i).index] - 0.0003;
		Tool.Color = RGB(255,255,0);  // Yellow
		//Tool.Text.Format("ATR Value: %f",atrValue );
		Tool.Text = "AHH";
		Tool.FontSize = 16;
		Tool.AddMethod = UTAM_ADD_OR_ADJUST;
		
		sc.UseTool(Tool);		
	}

	// draw 3T zone 
	for (int i = 0; i < detectDTWithGoodOF->size(); i++)   
	{
		//float max_high  = max(detectDTWithGoodOF->at(i).highOfPivotHigh , detectDTWithGoodOF->at(i).highOfGoodOF );
		float max_high = (detectDTWithGoodOF->at(i).highOfPivotHigh > detectDTWithGoodOF->at(i).highOfGoodOF) ? detectDTWithGoodOF->at(i).highOfPivotHigh : detectDTWithGoodOF->at(i).highOfGoodOF;
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDTWithGoodOF->at(i).indexOfPivotHigh;	
		rectangle.EndIndex = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;
		rectangle.BeginValue = max_high ;
		rectangle.EndValue = max_high - 3*sc.TickSize ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDTWithGoodOF->at(i).lineNumberOf3tzone ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
		
		// draw good sell OF
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;	
		rectangle.EndIndex = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar; 
		rectangle.BeginValue = detectDTWithGoodOF->at(i).highOfGoodOF ;
		rectangle.EndValue = detectDTWithGoodOF->at(i).lowOfGoodOF ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDTWithGoodOF->at(i).lineNumberOfGoodOf ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
	}

	
	 
	// ########################           5. write to file                  ################################################
	if(sc.UpdateStartIndex == 0)
	{
		SCString filePath = sc.DataFilesFolder() + "DT_with_good_OF.txt";   		

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
		
		
		
		outputFile << "detect Double top with good OF  : " << "\n" ;
		
		for (int i = 0; i < detectDTWithGoodOF->size(); i++)
		{
			float hmm = findMaxGoDownBeforeBreakHigh( sc , detectDTWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDTWithGoodOF->at(i).indexOfGoodOfEndBar  );
			int howMuchMove = sc.PriceValueToTicks(hmm) ;
			
		
			
			SCFloatArray vol_per_sec;
			sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
			float vpsOfEndBar = vol_per_sec[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] ;					
			
			int diff_idx_pivot_goodOF = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar - detectDTWithGoodOF->at(i).indexOfPivotHigh ;
			int diff_index_st_ed_GF_bar = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar - detectDTWithGoodOF->at(i).indexOfGoodOfStartBar  ;
			
			SCDateTime t = sc.BaseDateTimeIn[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar];
			SCString date = sc.DateTimeToString( t, FLAG_DT_COMPLETE_DATETIME);
			
			// for mean 5 last 5 vol , if idx = 10 , i = 9,8,7,6,5
			float mean5 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar, 20 ) ; 			//5
			// for mean 5 last 10 vol , if idx = 10 , i = 4,3,2,1,0
			float mean5_last_10 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar-20 , 20 ) ; //5,5
		
			outputFile << "i : " << i ; 
			outputFile << " | pv idx : " << detectDTWithGoodOF->at(i).indexOfPivotHigh  ; 
			outputFile << " | gf idx : " <<  detectDTWithGoodOF->at(i).indexOfGoodOfStartBar ;
			outputFile << " | diff idx PV GF : " << diff_idx_pivot_goodOF ;  
			outputFile << " | diff idx st ed GF : " << diff_index_st_ed_GF_bar ;  
			outputFile << " | VPS : " << vpsOfEndBar ;		
			outputFile << " | mean 5 : " << mean5 ;
			outputFile << " | mean 5 last 10 : " << mean5_last_10 ;
			outputFile << " | Move : " << howMuchMove << "T" ;
			outputFile << " | date : " << date ;	
			outputFile << "\n";
		}		
		
		// Close the file
		outputFile.close();
		
		
		
//      ################################################                        ############################################
//      ################################################ Get pattern and filter ############################################
//      ################################################                        ############################################


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
		SCFloatArray ATRArray;
		sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
				
		// for ema200
		SCFloatArray EMAArray;
		sc.GetStudyArrayFromChartUsingID(7 , 1 , 0, EMAArray);
		
		
		
		
					
		for (int i = 0; i < detectDTWithGoodOF->size(); i++)
		{
			// for get index on m5 chart
			int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectDTWithGoodOF->at(i).indexOfGoodOfEndBar);   // (chart number of m5 , sc.Index)
			
			SCDateTime t1 = sc.BaseDateTimeIn[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar];
			SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
			
			float hmm = findMaxGoDownBeforeBreakHigh( sc , detectDTWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDTWithGoodOF->at(i).indexOfGoodOfEndBar  );
			int howMuchMove = sc.PriceValueToTicks(hmm) ;
			
			int diff_pv_gf_idx = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar  - detectDTWithGoodOF->at(i).indexOfPivotHigh  ;

			// for atr
			float atrValue = ATRArray[RefChartIndex];
			
			// for ema200
			float emaValue = EMAArray[RefChartIndex];
		
			// for mean 5 last 5 vol , if idx = 10 , i = 9,8,7,6,5
			float mean20 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar, 20 ) ; //5
			
			// for mean 5 last 10 vol , if idx = 10 , i = 4,3,2,1,0
			float mean20_last_20 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar-20 , 20 ) ; //5
		
			// for mean delata
			float meanDelta5 = meanDelta(  sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar, 5 ) ;    //5,10
	
		
		
			// ################################################ Start filter DT and Good Sell OF Here  ############################################
			
		    //if(howMuchMove < 40) 	continue;		
			//if(t1.GetHour() < 7) continue;		
			//if(t1.GetHour() > 18) continue;	
			if(diff_pv_gf_idx < 3) continue;	
			if(diff_pv_gf_idx > 40) continue;	
			if(mean20 > mean20_last_20) continue;	
		    //if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex < 80 ) continue;		
		 	//if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex > 450 ) continue;
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < 1000 ) continue;				
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5/2 ) continue;	
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5*1 ) continue;	
			//if(meanDelta5*3 >  abs( delta[detectGoodSellOF->at(i).endBarIndex] )  ) continue;
			//if(meanMinDelta10 > -30 ) continue;			
			//if(meanMaxDelta10 > 100 ) //continue;					
			//if(sc.PriceValueToTicks(atrValue) > 1)  continue;			
			if(vol_per_sec[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] > 10)  continue;			//1.4
			//if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex >= 3 && vol[detectGoodSellOF->at(i).endBarIndex] <= 1000) //continue;			
			//if(sc.PriceValueToTicks( vwap[detectGoodSellOF->at(i).endBarIndex]-sc.Close[detectGoodSellOF->at(i).endBarIndex] ) <= -3)  continue;			
			//if(sc.PriceValueToTicks( vwap[detectGoodSellOF->at(i).endBarIndex]-sc.Close[detectGoodSellOF->at(i).endBarIndex] ) >= 3)  continue;
			//if(cum_vol_day[detectGoodSellOF->at(i).endBarIndex] > 20000)  continue;
	/*		int startIdx = detectGoodSellOF->at(i).startBarIndex ;
			int endIdx = detectGoodSellOF->at(i).endBarIndex ;
			int idxOfGoodSellOF = i ;
			bool haveStartBarIsSameAsBefore = false;
			float lowOfThisEndBar = sc.Low[endIdx] ;
			float volOfThisEndBar = detectGoodSellOF->at(i).volume_of_end_bar ;
			int sumOFSameStartIndex = 0;
			for(int j = idxOfGoodSellOF-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
			{
				
				// if find start bar index same as this index before then dont want
				if( (startIdx == detectGoodSellOF->at(j).startBarIndex)  )
				{
					
					if( lowOfThisEndBar < sc.Low[detectGoodSellOF->at(j).endBarIndex]  ) continue;  // if low of this end bar below low of previous end bar then it not same
					if(volOfThisEndBar < 50*detectGoodSellOF->at(j).volume_of_end_bar) continue; // if volOfThisEndBar more 10x of previous same start bar then it not same
					//if( detectGoodSellOF->at(j).volume_of_end_bar <= 20)   sumOFSameStartIndex++ ;
					
					sumOFSameStartIndex++ ;
				}
			}
			//if(sumOFSameStartIndex > 0 )  continue;
			int sumOFTouchHighOfStartIndex = 0;
			for(int k = endIdx-1 ; k >= startIdx+1; k--)
			{
				if(sc.Low[k] <= sc.High[startIdx]+sc.TickSize )   // if touch high of start bar index
				{
					sumOFTouchHighOfStartIndex++;
				}
			}
			//if(sumOFTouchHighOfStartIndex > 5 )  continue;  //3
	*/		
			// ########################################### End filter DT with Good Sell OF Here  ############################################
			
			
			// ######################## Add data to fillterDTwithGoodSellOF Object 
			int i1 = detectDTWithGoodOF->at(i).lineNumberOf3tzone;
			int i2 = detectDTWithGoodOF->at(i).lineNumberOfGoodOf;
			int i3 = detectDTWithGoodOF->at(i).indexOfPivotHigh;
			int i4 = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;
			int i5 = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar;
			float f1 = detectDTWithGoodOF->at(i).highOfPivotHigh;
			float f2 = detectDTWithGoodOF->at(i).highOfGoodOF;
			float f3 = detectDTWithGoodOF->at(i).lowOfGoodOF;
			
			fillterDTwithGoodSellOF->emplace_back(i1,i2,i3,i4,i5,f1,f2,f3);	
			
	
	       			
		
			
		
		}
	
	
		
		
		
		// ################################## SIM ###############################################
		simTrade( sc , *fillterDTwithGoodSellOF , *sim1s , tp_in_ticks) ;
		// ################################## SIM ###############################################
		
		
	}
	
	
	
}

// Function to find pivot highs
bool iSPivotHigh(SCStudyInterfaceRef sc , int index, int pivotLength) 
{
	bool isPivotHigh = true;
	
	if(index + pivotLength >= sc.ArraySize-1 )
	{
		return false ;
	}
	
	for (int i = 1; i <= pivotLength; i++) 
	{        
       if (sc.High[index] <= sc.High[index - i] || sc.High[index] <= sc.High[index + i]) {
                isPivotHigh = false;
                break;
        }
        
    }
    return isPivotHigh;
}

// Function to find pivot lows
bool iSPivotLow(SCStudyInterfaceRef sc , int index, int pivotLength) 
{
	bool isPivotLow = true;
	
	if(index + pivotLength >= sc.ArraySize-1 )
	{
		return false ;
	}
	
	for (int i = 1; i <= pivotLength; i++) 
	{        
       if (sc.Low[index] >= sc.Low[index - i] || sc.Low[index] >= sc.Low[index + i]) {
                isPivotLow = false;
                break;
        }
        
    }
    return isPivotLow;
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
	else if(condition == CHECK_5LEVEL_AND_POCATTOP)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
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
		
		if(levelOFHighestVol >= 3) 
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
	
	return false;
	
}


void findGoodSellOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , std::vector<st_GoodSellOF>& detectGoodSellOF) 
{
	int lowOfBar  ;
	int highOfBar ;
	int lowOfStartBar ;
	int highOfStartBar ;
			
    // loop from first index of start bar of good Sell OF to last 
	for (const auto& bar : detectedGoodSellStartBars) {
        int startBarIndex = bar.index;
		SCString msg;
		msg.Format("low of index %d is valid start bar | pRICE = %d  " , bar.index , sc.PriceValueToTicks(sc.Low[bar.index])  );
		//sc.AddMessageToLog(msg,0);
			
		// loop from start bar to current bar
        for (int j = startBarIndex + 1; j < sc.ArraySize - 1; ++j) 
		{
			// check if start bar and end bar far apart 1 Tick ?
			lowOfBar = sc.PriceValueToTicks(sc.Low[j]) ;
			highOfBar = sc.PriceValueToTicks(sc.High[j]) ;
			lowOfStartBar = sc.PriceValueToTicks(sc.Low[startBarIndex]) ;
			highOfStartBar = sc.PriceValueToTicks(sc.High[startBarIndex]) ;			
			
			
            if((highOfBar <= lowOfStartBar + 1) && (highOfBar >= lowOfStartBar - 1))
			{
			
				// check end bar is 5 level , poc on bottom , down candle
                if (checkValidBar(sc, j, CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE)) 
				{
					SCString msg;
					msg.Format("found valid poc at bottom and down candle start bar : %d | end bar : %d \n" , startBarIndex , j );
					//sc.AddMessageToLog(msg,0);
					// check no candle go above high of start bar
					bool noCandlebreakhigh = true;
					for(int k = startBarIndex ; k <= j ; ++k )
					{
						if(sc.High[k] > sc.High[startBarIndex])
						{
							noCandlebreakhigh = false ;
							break;
						}
					}					
					// draw if no candle break high of start bar
					if(noCandlebreakhigh)
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
						detectGoodSellOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
												
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

float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) 
{
	//float low = sc.Low[indexOfStartBar] - 0.0001 ;     // SL at low
	//float low = sc.High[indexOfEndtBar] - 0.0005 ;
    //int high = 	sc.PriceValueToTicks(sc.Low[indexOfEndtBar]) + 5 ;// SL at 
	int high = 	sc.PriceValueToTicks(sc.High[indexOfStartBar]) + 1 ;// SL at 
	
	int indexThatBreakHigh = -1;
	
	// find which index break high
	for(int i = indexOfEndtBar ; i < sc.ArraySize-1 ; i++)
	{
		if( sc.PriceValueToTicks(sc.High[i]) >= high )
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
	
	for(int i = indexOfEndtBar ; i <= indexThatBreakHigh ; i++)
	{
		if(sc.Low[i] < maxPriceGo)
		{
			maxPriceGo = sc.Low[i] ;
		}
	}
	
	return sc.Low[indexOfEndtBar]-maxPriceGo  ;
	
}




void simTrade(SCStudyInterfaceRef sc , std::vector<st_DoubleTopWithGoodOF>& detectDTwithGoodSellOF , std::vector<st_SimTrade>& detectTrade , int tp_in_ticks) 
{
	int tradeNumber = 0 ;
	float tp  ;
	
	
	detectTrade.clear() ;
	
	

	
		
	// loop in good Sell OF
	for (int i = 0; i < detectDTwithGoodSellOF.size(); i++)
	{
		int sellIndex = i;
		
		int entryIndex = detectDTwithGoodSellOF[i].indexOfGoodOfEndBar ;
		float entryPrice = detectDTwithGoodSellOF[i].lowOfGoodOF ;
		float stopLoss = detectDTwithGoodSellOF[i].highOfGoodOF + sc.TickSize ;
		tp = entryPrice - tp_in_ticks*sc.TickSize  ;
		
		int indexOfPattern ;
		int result ;
		
		// loop from entry bar to end
		for(int j = entryIndex+1 ; j < sc.ArraySize-1; j++)
		{
			// if break low win
			if(sc.Low[j] <= tp)
			{
				// win
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = tp_in_ticks ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
			    
				break;				
												
				
			}// if breaK High loss
			else if( sc.High[j] >= stopLoss)
			{
				// loss
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = sc.PriceValueToTicks(entryPrice-stopLoss) ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
				break;		
			}
		}
		
		
	}
	
	
	
	
	SCString filePath = sc.DataFilesFolder() + "Sim DT with good OF Trades.txt";   		

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
		int idx = detectTrade[i].indexOfPattern ;
		int pivotIndex = detectDTwithGoodSellOF[idx].indexOfPivotHigh ;
		int startIndex = detectDTwithGoodSellOF[idx].indexOfGoodOfStartBar ;
		int endIndex =  detectDTwithGoodSellOF[idx].indexOfGoodOfEndBar ;
		int rs = detectTrade[i].result ;
		
		float hmm = findMaxGoDownBeforeBreakHigh( sc , startIndex ,endIndex  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		
		
		float mean20 = meanVol( sc , endIndex, 20 ) ; 			//5		
		float mean20_last_20 = meanVol( sc , endIndex-20 , 20 ) ; //5,5
		
		// it is vol/sec
		SCFloatArray vol_per_sec;
		sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
		float vps = vol_per_sec[endIndex] ; 
		
		SCDateTime t1 = sc.BaseDateTimeIn[detectDTwithGoodSellOF[i].indexOfGoodOfEndBar];
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
		
		outputFile << "Trade: " << detectTrade[i].tradeNumber ;
		outputFile << ", pivot index : " << pivotIndex ;
		outputFile << ", start index : " << startIndex ;
		outputFile << ", end index : " << endIndex ;
		outputFile << ", vps : " << vps ;
		outputFile << ", mean20 : " << mean20 ;
		outputFile << ", mean 20 last 20 : " << mean20_last_20 ;
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
