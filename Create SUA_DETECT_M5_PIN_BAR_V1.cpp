// The top of every source code file must include this line
#include "sierrachart.h"

// For reference, refer to this page:
// https://www.sierrachart.com/index.php?page=doc/AdvancedCustomStudyInterfaceAndLanguage.php

// This line is required. Change the text within the quote
// marks to what you want to name your group of custom studies. 
SCDLLName("SUA_DETECT_M5_PIN_BAR")

//This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
SCSFExport scsf_detect_m5_pin_bar(SCStudyInterfaceRef sc)
{
	
	SCString msg;	
	SCInputRef i_m5_chart_number = sc.Input[0];
	SCInputRef i_multiply_body_size = sc.Input[1];
	SCInputRef i_multiply_pin_size_of_other_pin_size = sc.Input[2];
	
	// **************** Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "DETECT M5 PIN BAR";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		i_m5_chart_number.Name = "M5 Chart Number";
        i_m5_chart_number.SetInt(7);  // Default to chart number 7
		
		i_multiply_body_size.Name = "number of what times you want body size than pin size" ;
		i_multiply_body_size.SetInt(2);
		
		i_multiply_pin_size_of_other_pin_size.Name = "number of what times you want pin size than anoher pin size" ;
		i_multiply_pin_size_of_other_pin_size.SetInt(2);
		
		return;
	}
	
	
	// **************** Section 2 - Do data processing here
	
	
	
	// Get number from input
	int M5ChartNumber = i_m5_chart_number.GetInt();
	int multiply_body_size = i_multiply_body_size.GetInt();
	int multiply_pin_size_of_other_pin_size = i_multiply_pin_size_of_other_pin_size.GetInt();
	
	
	int startIndex;
	
	// reload , recalculate situlation
	if(sc.Index == 0)
	{
		startIndex = 0;
	}
	else // live
	{
		startIndex = sc.UpdateStartIndex;
	}
	
	
	// loop here 
	 for (int i = startIndex; i < sc.ArraySize; ++i) 
	{
		
		// Check if this is a new bar
		if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED  ) {
			continue;
		}			
		
		 // Variables to store M5 bar data
		SCGraphData m5_data;
		SCDateTimeArray m5_date;
		SCFloatArray m5_open, m5_high, m5_low, m5_close, m5_volume;
		// Get the historical data from the M5 chart
		sc.GetChartBaseData(M5ChartNumber, m5_data);
		sc.GetChartDateTimeArray(M5ChartNumber, m5_date);
		
		m5_high = m5_data[SC_HIGH];
		m5_low = m5_data[SC_LOW];
		m5_open = m5_data[SC_OPEN];
		m5_close = m5_data[SC_LAST];
		
		// Get the index in the specified chart that is
		// nearest to current index.
		int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, sc.Index);   // (chart number of m5 , sc.Index)
		
		
	
     	// Example of processing the data: Log the latest M5 bar data to the Message Log
		int m5_currentIndex = m5_date.GetArraySize() - 1;
		
		
		
		//SCString IndexDateTime = sc.DateTimeToString(m5_date[m5_currentIndex-1], FLAG_DT_COMPLETE_DATETIME);
		//msg.Format("************ M5 high of index i = %f" , m5_high[m5_lastIndex-1] );
		//sc.AddMessageToLog(msg,0);
		
		//sc.AddMessageToLog(IndexDateTime, 0);
		
		// now check pin bar
		// find body , pin size
		float body_size , top_pin_size , bottom_pin_size;
		bool is_up_candle ;
		enum candle_direction { up, down, unknow };
		
		candle_direction pin_bar ;
		
		int m5_index = m5_currentIndex-1;
		
		if(m5_open[m5_index]-m5_close[m5_index] > 0)  // down candle
		{
			body_size = m5_open[m5_index]-m5_close[m5_index] ;
			top_pin_size = m5_high[m5_index] - m5_open[m5_index];
			bottom_pin_size = m5_close[m5_index] - m5_low[m5_index];
			//direction = down;
			
		}
		else if(m5_open[m5_index]-m5_close[m5_index] < 0) // up candle
		{
			body_size = m5_close[m5_index]-m5_open[m5_index] ;
			top_pin_size = m5_high[m5_index] - m5_close[m5_index];
			bottom_pin_size = m5_open[m5_index] - m5_low[m5_index];
			//direction = up;
		}
		else  // equal body
		{
			//direction = unknow;
		}
		
		/*sc.GetChartArray(SC_LAST, M5ChartNumber, m5_close); 
		
		
		int x = m5_close.GetArraySize();
		msg.Format("************ M5 array size = %d ",x );
		sc.AddMessageToLog(msg,0);*/

		msg.Format("************ M5 high of index-1  = %f" , m5_high[m5_index] );
		sc.AddMessageToLog(msg,0);
		msg.Format("************ body size = open - close = %f - %f = %f " , m5_open[m5_index] ,m5_close[m5_index] , body_size  );
		sc.AddMessageToLog(msg,0);
		msg.Format("************ top pin size = %f" , top_pin_size );
		sc.AddMessageToLog(msg,0);
		msg.Format("************ bottom pin size = %f" , bottom_pin_size );
		sc.AddMessageToLog(msg,0);
		
		SCString IndexDateTime = sc.DateTimeToString(m5_date[m5_index], FLAG_DT_COMPLETE_DATETIME);
		sc.AddMessageToLog(IndexDateTime, 0);
		
		
		if( (top_pin_size >= body_size * multiply_body_size) || (bottom_pin_size >= body_size * multiply_body_size) )  // pin must more than body*2   
		{
			if(top_pin_size >= bottom_pin_size*multiply_pin_size_of_other_pin_size)  
			{
				pin_bar = down;
				msg.Format("found down pin bar high = %f" , m5_high[m5_index] );
				
				 // Use sc.UseTool to change the bar color to yellow
				s_UseTool Tool;
				Tool.Clear();
				Tool.ChartNumber = 7;
				Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
				Tool.AddAsUserDrawnDrawing = 1;
				Tool.SecondaryColor = RGB(255, 255, 0);  // Yellow color
				Tool.BeginIndex = m5_index-1;
				Tool.EndIndex = m5_index+1;
				Tool.BeginValue = m5_low[m5_index];
				Tool.EndValue = m5_high[m5_index];
				Tool.Color = RGB(255, 255, 0);  // Yellow color
				Tool.TransparencyLevel = 50;    // Set transparency level if needed
				Tool.AddMethod = UTAM_ADD_OR_ADJUST;
				Tool.LineNumber = 78879 + i;
				Tool.AllowCopyToOtherCharts = true;
				
				sc.UseTool(Tool);
			}
			else if(bottom_pin_size >= top_pin_size*multiply_pin_size_of_other_pin_size)
			{
				pin_bar = up;
				msg.Format("found up pin bar low = %f" , m5_low[m5_index] );
				
				 // Use sc.UseTool to change the bar color to yellow
				s_UseTool Tool;
				Tool.Clear();
				Tool.ChartNumber = 7;
				Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
				Tool.AddAsUserDrawnDrawing = 1;
				Tool.SecondaryColor = RGB(255, 255, 0);  // Yellow color
				Tool.BeginIndex = m5_index-1;
				Tool.EndIndex = m5_index+1;
				Tool.BeginValue = m5_low[m5_index];
				Tool.EndValue = m5_high[m5_index];
				Tool.Color = RGB(255, 255, 0);  // Yellow color
				Tool.TransparencyLevel = 50;    // Set transparency level if needed
				Tool.AddMethod = UTAM_ADD_OR_ADJUST;
				Tool.LineNumber = 78879 + i;
				Tool.AllowCopyToOtherCharts = true;
				
				sc.UseTool(Tool);
			}
			else  // top_pin_size == bottom_pin_size
			{
				pin_bar = unknow;
				msg.Format("pin size not more than 2x of other pin sidz");
			}
			sc.AddMessageToLog(msg,0);			
			
			
		}
		else
		{
			msg.Format("pin size less than 2*body");
			sc.AddMessageToLog(msg,0);	
		}
		
	
	}
		
	
}
