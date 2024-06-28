// The top of every source code file must include this line
#include "sierrachart.h"
#include <vector>            // For using std::vector
#include <sstream>           // For string manipulation


#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  
#define MAX_VECTOR_SIZE 500

int uniqueNumber = 7889 ;

// Structure to represent an good buy OF start bar
struct st_GoodBuyOFStartBar {
    int index;
    SCDateTime time;
    float volume;
	int lineNumber;
  
	st_GoodBuyOFStartBar(int idx, SCDateTime t, float v , int ln) : index(idx), time(t), volume(v) , lineNumber(ln) {}
};

int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars);

//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;


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
	
	// Get the M5 chart number from input
    int M5ChartNumber = i_m5_chart_number.GetInt();
	
		
	
	auto detectedGoodBuyStartBars = static_cast<std::vector<st_GoodBuyOFStartBar>*>(sc.GetPersistentPointer(1));
	
	if (!detectedGoodBuyStartBars) {
        detectedGoodBuyStartBars = new std::vector<st_GoodBuyOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodBuyStartBars);
    }
	
	
	int minVol = i_minVol.GetInt();
	int startIndex = sc.UpdateStartIndex;
	int length = 1;
	
 
	//A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		for (int i = 0; i < detectedGoodBuyStartBars->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectedGoodBuyStartBars->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectedGoodBuyStartBars->clear();

			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (detectedGoodBuyStartBars != NULL)
		detectedGoodBuyStartBars->clear();
	
	
	
	
	 // 1.Loop through bars to detect good buy start bar pattern
    for (int i = startIndex; i < sc.ArraySize; ++i) 
	{
		
		// Check if this is a new bar
		if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) {
            continue;
        }
		
		
		
		bool isItStartBarOFGoodBuyOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM);
		
		
		// if found good buy start bar , then add it to vector
		if(isItStartBarOFGoodBuyOrderflow)
		{
			SCDateTime time = sc.BaseDateTimeIn[i];
            float vol = sc.Volume[i];
            int lineNm = uniqueNumber + i;
			
			// use emplace_back
			detectedGoodBuyStartBars->emplace_back(i, time, vol , lineNm);

			
			
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodBuyStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodBuyStartBars->erase(detectedGoodBuyStartBars->begin());
			}
		
		}
		
		// 2. find end bar of good buy OF
		//findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars);
		
	
	}
	
	 // 2. find end bar of good buy OF
	findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars);
	
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

    // Write the data to the file
    outputFile << "Bar Index: " << "\n";

    // Close the file
    outputFile.close();

	
	
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


void findGoodBuyOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars) 
{
    // loop from first index of start bar of good buy OF to last 
	for (/*const*/ auto& bar : detectedGoodBuyStartBars) {
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
						bar.lineNumber = uniqueNumber + startBarIndex + j ;
						
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
						rectangle.LineNumber = bar.lineNumber ;
						sc.UseTool(rectangle);
						
						SCString msg;
						msg.Format("start bar : %d | end bar : %d \n" , startBarIndex , j );
						sc.AddMessageToLog(msg,0);
					}
                 
                }
            }
        }
    }
}


int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex)
{
	
	
	return -1 ;
}


