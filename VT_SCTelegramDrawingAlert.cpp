// need to add these dll files to C:\SierraChart\Data too
// VT_SCTelegramDrawingAlert.dll 
// zlib.dll 
// libcrypto-3-x64.dll 
// libcurl.dll 
// libssh2.dll 
// libssl-3-x64.dll


#include <boost/format.hpp>
#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <thread>
#include "json.hpp" // convenience 

#include "sierrachart.h"

// const int StudyVersion = 136 
// Last Updated Thu Aug  1 11:06:26 EDT 2024
SCDLLName("Sua -Verrillog - Telegram Chart Drawing Alerts")
////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetLogsFolderPath(SCStudyInterfaceRef sc, std::string& LogsFolderPath)
{
	// Get the directory of sierra data folder from a function directly into std::string
	LogsFolderPath = sc.DataFilesFolder().GetChars();

	// Get the Starting position of 'SierraChart' text from Data Folder Path 
	std::size_t start_pos = LogsFolderPath.find("SierraChart");

	// find the first backslash after SierraChart (adds support for
	// Sierrachart2,3,4, or any text succeeding it)
	std::size_t end_pos = LogsFolderPath.find('\\', start_pos);
	if (start_pos != std::string::npos)
	{
		// Create a string from the existing string that omits the "Data" characters
		// and adds the "Logs" characters to the end of it. 

		// re assign our directory string to the new value
		LogsFolderPath = LogsFolderPath.substr(0, end_pos + 1) + "Logs";
	}
	else
	{
		// If we get here it means the Data Files Folder path does not have
		// SierraChart in it.  The user moved it to some other directory. 
		// This means the log file path needs to be specified through the study
		// input.
		//
		sc.AddMessageToLog("Error: text 'SierraChart' not found in Data Folder Path."
			" Please enable Study input #6 and specify the full path to Sierra Chart Install directory in Study Input #7.", 1);

		LogsFolderPath = "Invalid_Path";
	}
}
void FindMostRecentFile(SCStudyInterfaceRef sc, const std::string& LogsFolderPath, int& number_of_files,
	int64_t& Local_LastModTime, std::string& most_recent_filename)
{
	// HANDLE if the folder does not exist before accessing the directory 
	if (!std::filesystem::exists(LogsFolderPath) && !std::filesystem::is_directory(LogsFolderPath))
	{
		sc.AddMessageToLog("Error: Logs Folder does not exist! Check if the folder exists in SC install directory."
			" Enable Save Alerts Log to File under General Settings > Log. Then trigger a chart drawing alert and "
			"the directory should be created.", 1);
		return;
	}

	// 1. Iterate through the directory to find the number of files that match
	// 2. Find the file with the most recent modification time
	for (const auto& entry : std::filesystem::directory_iterator(LogsFolderPath))
	{
		if (entry.is_regular_file())
		{
			// save the file name
			std::string filename = entry.path().filename().string();

			// Make sure the file is an Alert Log and the last 3 characters are 'log'
			// to ensure it's not a swap file or some other extension
			if (filename.find("Alert Log") != std::string::npos &&
				filename.substr(filename.length() - 3) == "log")
			{
				// count the number of files in directory
				number_of_files++;

				// get the last write time of our file	
				std::filesystem::file_time_type CurrentModTime = std::filesystem::last_write_time(entry);

				// convert it to a time in seconds from epoch
				auto CurrentModTimePoint = std::chrono::duration_cast<std::chrono::seconds>
					(CurrentModTime.time_since_epoch()).count();

				//	if this is the first time the study function is run 
				//	save it to check on the next iteration (HANDLES only 1 file present)
				if (Local_LastModTime == 0)
				{
					// save the last modified time
					Local_LastModTime = CurrentModTimePoint;

					// save the filename so we can potentially read it later
					most_recent_filename = std::move(filename);
				}
				else
				{
					// if there is only one FILE we will not get here!
					// We should get here on the following iterations

					// Check if the file modified time is more recent than the last one
					if (CurrentModTimePoint > Local_LastModTime)
					{

						// save the last modified time
						Local_LastModTime = CurrentModTimePoint;

						// save the filename
						most_recent_filename = std::move(filename);

						// debug
						/* msg.Format("print filenames: %s", filename.c_str()); */
						/* sc.AddMessageToLog(msg, 1); */
					}
				}
			}
		}
	}
}
void FindDuplicateStudiesInSameChartbook(SCStudyInterfaceRef sc, const char* StudyName, SCString& msg)
{
	// Get the highest chart number in the current chartbook 
	int highest_chart_num = sc.GetHighestChartNumberUsedInChartBook();

	// get the current chart number for reference
	int this_chart_num = sc.ChartNumber;

	// vector used to save the chart numbers of charts in current chartbook
	std::vector <int> chart_numbers;

	// Go through each chart number from 1 to highest chart number to 
	// determine the chart numbers that exist and save those. 
	for (int ChartNumber = 1; ChartNumber <= highest_chart_num; ChartNumber++)
	{
		// returns true if chart number exists in current chartbook, empty string refers to current chartbook
		if (sc.IsChartNumberExist(ChartNumber, ""))
		{
			// add this chart number to the vector 
			chart_numbers.push_back(ChartNumber);
		}
	}

	// iterate through the existing number of charts with chart numbers as values  
	for (int i = 0; i < chart_numbers.size(); i++)
	{
		// Check if the study with this name is found on this chart number
		// arguments: (chart number, study name as a string, search for study short name instead)
		int is_study_found = sc.GetStudyIDByName(chart_numbers[i], StudyName, 0);

		// If the study is found and if the chart it was found on is not the current chart
		if (is_study_found != 0 && chart_numbers[i] != this_chart_num)
		{
			// This should only return true if the study exists on two or more chartbooks
			// print which charts where the study is found
			msg.Format("A duplicate is found on chart #%d. Reduce the number of studies "
				"per chartbook to one unless you wish to recieve duplicate alerts.", chart_numbers[i]);
			sc.AddMessageToLog(msg, 1);
		}
	}
}

void ParseChartStudyAlertText(std::string& line, std::string& SourceChartImageText, int& CustomizeMessageContent,
	SCString& sc_alert_chartbook, int& sc_alert_chart_num)
{
	// String parsing logic 
	// find various text inside the string
	//
	// Get the Source Chart Text String
	std::size_t source_start_pos = line.find("Source");
	std::size_t source_end_pos = line.find('|', source_start_pos);

	// Alert text string (different for Chart Alerts)
	std::size_t text_start_pos = line.find("Formula");
	std::size_t first_pipe_pos = line.find('|', text_start_pos);

	// set the end pos to the second pipe character to capture the alert condition that was triggered.
	// Necessary to set the starting position 1 in front or else it will just find the same one 
	std::size_t text_end_pos = line.find('|', first_pipe_pos + 1);

	// REMOVE / characters from Source Chart Text
	// check over the substring for any forward slash character and kill loop if we reach npos string position 
	for (auto i = source_end_pos; i != source_start_pos && i != std::string::npos; i--)
	{
		if (line[i] == '/')
		{
			// Replace the character at the position i with a whitespace 
			//string& replace (size_t pos, size_t len, const string& str);
			line.replace(i, 1, " ");
		}
	}

	// Do the same for the Alert Text String
	for (auto i = text_end_pos; i != text_start_pos && i != std::string::npos; i--)
	{
		if (line[i] == '/')
		{
			// Replace the character at the position i with a whitespace 
			line.replace(i, 1, " ");
		}
	}

	// CREATE image file text using the source chart text iterators 
	SourceChartImageText = line.substr(source_start_pos + 8, source_end_pos - (source_start_pos + 8));

	// REMOVE INVALID CHARACTERS FROM Image File Text for WINDOWS FILENAME 
	// an array of invalid characters for a windows filename
	std::array<char, 9> invalid_filename_characters = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };

	// for each of the invalid characters 
	for (int index = 0; index < invalid_filename_characters.size(); index++)
	{
		// iterate backwards through the string so we can delete characters from it 
		for (auto i = SourceChartImageText.end(); i != SourceChartImageText.begin(); i--)
		{
			// if current string character is equal to the current element in invalid characters array 
			if (*i == invalid_filename_characters[index])
			{
				// erase this character from the string
				/* SourceChartImageText.erase(i); */
				// replace the invalid character with a whitespace 
				*i = ' ';
			}
		}
	}

	// Determine specifically if it is a Chart or Study alert 
	// This will be done further down by comparing the source string with study text string

	// This iterator gives us a starting point to retrieve the Study Name (short name). 
	std::size_t is_studyalert_start_pos = line.rfind("Study:", text_start_pos);

	std::size_t study_name_start_pos = 0;
	std::size_t study_name_end_pos = 0;

	// 2024-01-05 Due to an inconsistency found with studies that have Use As Main Price
	// Graph enabled For now it is necessary to perform a check that will ensure the
	// study name is retrieved safely. 
	//
	// This variable is used to inform the program of the inconsistency 
	bool format_inconsistency = 0;

	// if the iterator returns null it means that Display As Main Price Graph was enabled for the study.
	// When that is enabled it changes the formatting of the Alert Log to not include the Study: Study Name
	// it will be necessary to format the message differently further below
	if (is_studyalert_start_pos == std::string::npos)
	{
		// set the boolean to positive informing the study to use the separate formatting method 
		format_inconsistency = 1;
	}
	else
	{
		// if we get here we can continue to initialize the other variables safely 
		// set the starting point for the Study Name 7 characters infront of text: "Study: "
		study_name_start_pos = is_studyalert_start_pos + 7;

		// Set the end point of the substring
		study_name_end_pos = line.find('|', study_name_start_pos) - 1;
	}

	// Create our substring (safely since if they are initialized and it should not use invalid iterators)
	std::string_view study_name(line.c_str() + study_name_start_pos, study_name_end_pos - study_name_start_pos);

	// potentially have something here to take care of any special HTML text formatting

	// continue with the other substrings
	std::size_t bar_datetime_start_pos = line.find("Bar start:");
	std::size_t bar_datetime_end_pos = line.find('|', bar_datetime_start_pos) - 5; // removing the ms timestamp

	// only need the start position since it reads until end of the string
	std::size_t chartbook_start_pos = line.find("Chartbook:");

	// SAVE the alert log chartbook name independantly 
	std::string_view alert_chartbook(line.c_str() + chartbook_start_pos + 11);

	// Get the chart number where the alert originated from
	std::size_t find_chtbooknum = line.find("ChartNumber: ");
	std::size_t cht_number_start_pos = line.find(' ', find_chtbooknum) + 1;
	std::size_t cht_number_end_pos = line.find(' ', cht_number_start_pos);

	// save only the chart number itself  
	std::string_view alert_cht_number(line.c_str() + cht_number_start_pos,
		cht_number_end_pos - cht_number_start_pos);

	// Alert chart number must persist past the string formatting we are about to do 
	std::string alert_chart_number = std::string(alert_cht_number);

	if (source_start_pos != std::string::npos) // safety check
	{
		// std::string_view Make reference to different sequences of characters from the underlying string
		// without copying any of the data (lightweight) 
		std::string_view source_string(line.c_str() + source_start_pos, source_end_pos - source_start_pos);

		// Determine if source string and study name string are identical. This will tell us if the alert was from
		// a Chart Alert or a Study Alert.

		// Only check if it's a Chart or Study Alert if our previous substring got initialized (aka normal behaviour)
		std::size_t find_substring_match = 0;
		if (study_name_start_pos != 0)
		{
			// find the study_name contents inside of source_string. 
			// If a full match is found (iterator set to a valid number), this was a Chart Alert.
			find_substring_match = source_string.find(study_name);
		}

		bool is_chart_alert = 0;
		// check if the find_substring_match iterator was initialized and it was not initialized to null
		if (find_substring_match != 0 && find_substring_match != std::string::npos)
		{
			// debug 
			/* sc.AddMessageToLog("The Study Name is found within Source String, this was a Chart Alert.",1); */

			// informs the following code to format it as a chart alert instead 
			is_chart_alert = 1;

		}

		// set the alert text substring (using std::string because this string 
		// might need to be modified.
		//
		// With Chart/Study Alerts, this is where the formula is found. 
		std::string text_string(line.c_str() + text_start_pos, text_end_pos - text_start_pos);

		// find any < characters in the formula and replace them with html equivalent
		// this is a fix for where the < character is being interpreted as an opening
		// brace for html formatting. We want to display the literal character instead. 
		std::size_t find_less_than = text_string.find('<');
		if (find_less_than != std::string::npos)
		{
			// the < character is found 
			//
			// Replace all occurrences of '<' with '&lt;'
			std::size_t pos = 0;
			while ((pos = text_string.find('<', pos)) != std::string::npos)
			{
				text_string.replace(pos, 1, "&lt;");
				pos += 4; // Move past the replaced '&lt;'
			}
		}

		// set the date_time substring	
		std::string_view bar_datetime_string(line.c_str() + bar_datetime_start_pos,
			bar_datetime_end_pos - bar_datetime_start_pos);

		// set the chartbook substring
		std::string_view chartbook_string(line.c_str() + chartbook_start_pos);

		// Get the Input selection for Customize Message Text
		int CustomizeMessageSettingIndex = CustomizeMessageContent;

		// Format the string to pass in Telegram message
		if (format_inconsistency == 1) // Will remove when SC fixes bug
		{
			// Different formatting to adjust for the lack of Study Name 
			if (CustomizeMessageSettingIndex == 0) // default include formula
			{
				boost::format fmt = boost::format("%1%\n%2%\n%3%\n%4%")
					% text_string % source_string % bar_datetime_string % chartbook_string;

				// overwrite our line string with the formatted string
				line = fmt.str();
			}
			else if (CustomizeMessageSettingIndex == 1) // only remove formula
			{
				boost::format fmt = boost::format("%1%\n%2%\n%3%")
					% source_string % bar_datetime_string % chartbook_string;

				// overwrite our line string with the formatted string
				line = fmt.str();
			}
			else if (CustomizeMessageSettingIndex == 2) // same as no formula
			{
				boost::format fmt = boost::format("%1%\n%2%\n%3%")
					% source_string % bar_datetime_string % chartbook_string;

				// overwrite our line string with the formatted string
				line = fmt.str();
			}
		}
		else
		{
			if (is_chart_alert == 1)
			{
				// Formatting for Chart Alert 
				if (CustomizeMessageSettingIndex == 0) // default include formula
				{
					boost::format fmt = boost::format("%1%\n%2%\n%3%\n%4%")
						% text_string % source_string % bar_datetime_string % chartbook_string;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
				else if (CustomizeMessageSettingIndex == 1) // only remove formula
				{
					boost::format fmt = boost::format("%1%\n%2%\n%3%")
						% source_string % bar_datetime_string % chartbook_string;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
				else if (CustomizeMessageSettingIndex == 2) // same as no formula
				{
					boost::format fmt = boost::format("%1%\n%2%\n%3%")
						% source_string % bar_datetime_string % chartbook_string;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
			}
			else
			{
				// Formatting for Study Alert 
				if (CustomizeMessageSettingIndex == 0) // default
				{
					/* boost::format fmt = boost::format("<strong>%1%</strong>\n\n%2%\n%3%\n%4%\n%5%") */
					boost::format fmt = boost::format("%1%\n\n%2%\n%3%\n%4%\n%5%")
						% study_name % text_string % source_string % bar_datetime_string % chartbook_string;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
				else if (CustomizeMessageSettingIndex == 1) // only remove formula
				{
					/* boost::format fmt = boost::format("<strong>%1%</strong>\n\n%2%\n%3%\n%4%") */
					boost::format fmt = boost::format("%1%\n\n%2%\n%3%\n%4%")
						% study_name % source_string % bar_datetime_string % chartbook_string;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
				else if (CustomizeMessageSettingIndex == 2) // only include Study Name
				{
					/* boost::format fmt = boost::format("<strong>%1%</strong>") */
					boost::format fmt = boost::format("%1%")
						% study_name;

					// overwrite our line string with the formatted string
					line = fmt.str();
				}
			}
		}
	}

	// Convert Chartbook name as std::string_view into SCString (fancy 1 liner)
	sc_alert_chartbook = std::string(alert_chartbook).c_str();

	// Cast/Convert chart number stringview into int 
	sc_alert_chart_num = std::stoi(std::string(alert_cht_number));
}

void ParseChartDrawingAlertText(std::string& line, std::string& SourceChartImageText, int& CustomizeMessageContent,
	SCString& sc_alert_chartbook, int& sc_alert_chart_num)
{
	// String parsing for Chart Drawing Alerts
	// find various text inside the string that was obtained from the alert log file 
	//
	// Get the Source Chart Text String
	std::size_t source_start_pos = line.find("Source");
	std::size_t source_end_pos = line.find('|', source_start_pos);

	// Alert text string
	std::size_t text_start_pos = line.find("Chart Drawing");
	std::size_t text_end_pos = line.find('|', text_start_pos);

	// REMOVE / from Source Chart Text
	// check over the substring for any forward slash character and kill loop if we reach npos string position 
	for (auto i = source_end_pos; i != source_start_pos && i != std::string::npos; i--)
	{
		if (line[i] == '/')
		{
			// Replace the character at the position i with a whitespace 
			//string& replace (size_t pos, size_t len, const string& str);
			line.replace(i, 1, " ");
		}
	}

	// do the same for the Alert Text String
	for (auto i = text_end_pos; i != text_start_pos && i != std::string::npos; i--)
	{
		if (line[i] == '/')
		{
			// Replace the character at the position i with a whitespace 
			line.replace(i, 1, " ");
		}
	}

	// CREATE image file text using the source chart text iterators 
	SourceChartImageText = line.substr(source_start_pos + 8, source_end_pos - (source_start_pos + 8));

	// REMOVE INVALID CHARACTERS FROM Image File Text for WINDOWS FILENAME 
	// an array of invalid characters for a windows filename
	std::array<char, 9> invalid_filename_characters = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };

	// for each of the invalid characters 
	for (int index = 0; index < invalid_filename_characters.size(); index++)
	{
		// iterate backwards through the string so we can delete characters from it 
		for (auto i = SourceChartImageText.end(); i != SourceChartImageText.begin(); i--)
		{
			// if current string character is equal to the current element in invalid characters array 
			if (*i == invalid_filename_characters[index])
			{
				// erase this character from the string
				/* SourceChartImageText.erase(i); */
				// replace the invalid character with a whitespace 
				*i = ' ';
			}
		}
	}


	// MONOSPACE FORMATTING TO THE PRICE in HTML FORMAT
	// Get Iterators to the Price (there are two prices in the alert text)

	// We start from the end of the string and work our way back (safest way)
	std::size_t price2_end_pos = line.rfind(')', text_end_pos);
	std::size_t price2_start_pos = line.rfind('(', price2_end_pos);

	// Insert text for price2 formatting 
	line.insert(price2_end_pos, "</code>");
	line.insert(price2_start_pos + 1, "<code>");

	// reset the iterator for price2_start_pos in order to safely get price1 iterators
	price2_start_pos = line.rfind('(', text_end_pos);

	// Get the iterators for price1
	std::size_t price1_end_pos = line.rfind(')', price2_start_pos);
	std::size_t price1_start_pos = line.rfind('(', price1_end_pos);

	// Insert text for price1 formatting
	line.insert(price1_end_pos, "</code>");
	line.insert(price1_start_pos + 1, "<code>");

	// once done inserting the text, RESET Alert Text End Position to the new correct iterator
	text_end_pos = line.find('|', text_start_pos);

	// continue with the other substrings
	std::size_t bar_datetime_start_pos = line.find("Bar date-time");
	std::size_t bar_datetime_end_pos = line.find('|', bar_datetime_start_pos);

	// only need the start position since it reads until end of the string
	std::size_t chartbook_start_pos = line.find("Chartbook");

	// SAVE the alert log chartbook name independantly 
	std::string_view alert_chartbook(line.c_str() + chartbook_start_pos + 11);

	// Get the chart number where the alert originated from
	std::size_t find_chtbooknum = line.find("ChartNumber: ");
	std::size_t cht_number_start_pos = line.find(' ', find_chtbooknum) + 1;
	std::size_t cht_number_end_pos = line.find(' ', cht_number_start_pos);

	std::string_view alert_cht_number(line.c_str() + cht_number_start_pos,
		cht_number_end_pos - cht_number_start_pos);

	// Alert chart number must persist past the string formatting we are about to do 
	std::string alert_chart_number = std::string(alert_cht_number);

	// Chart Drawing Alerts are simpler than Study Alerts and do not have a formatting option 
	if (source_start_pos != std::string::npos) // safety check
	{
		// make reference to different sequences of characters from the underlying string
		// without copying any of the data (lightweight) 
		std::string_view source_string(line.c_str() + source_start_pos, source_end_pos - source_start_pos);
		std::string_view text_string(line.c_str() + text_start_pos, text_end_pos - text_start_pos);

		std::string_view bar_datetime_string(line.c_str() + bar_datetime_start_pos,
			bar_datetime_end_pos - bar_datetime_start_pos);
		std::string_view chartbook_string(line.c_str() + chartbook_start_pos);

		// Format the string to pass in Telegram message
		boost::format fmt = boost::format("%1%\n\n%2%\n%3%\n%4%") % text_string
			% bar_datetime_string % chartbook_string % source_string;

		// overwrite our line string with the formatted string
		line = fmt.str();
	}

	// Convert Chartbook name as std::string_view into SCString (fancy 1 liner)
	sc_alert_chartbook = std::string(alert_chartbook).c_str();

	// Cast/Convert chart number stringview into int 
	sc_alert_chart_num = std::stoi(std::string(alert_cht_number));
}

void SCTelegramPostRequest(SCStudyInterfaceRef sc, const SCString& host, const SCString& token, const std::string& ChatID, const std::string& line, SCString& msg)
{
	SCString method = "/sendMessage?";

	// Set our Telegram URL for the POST request 
	SCString URL = std::move(host) + std::move(token) + std::move(method);

	// SIERRA CHART POST REQUEST WITH JSON BODY EXAMPLE 
	//
	// Create the json object for Telegram
	// For simplicity using nlohmann json
	nlohmann::json object = {
	{"chat_id", std::move(ChatID)},
	{"text", std::move(line)},
	{"parse_mode", "HTML"},
	};

	// Convert nlohmann json into SCString in one line. 
	SCString query = object.dump().c_str();

	// Set our headers 
	n_ACSIL::s_HTTPHeader headers[1];
	headers[0].Name = "Content-Type";
	headers[0].Value = "application/json";

	if (sc.MakeHTTPPOSTRequest(URL, query, headers, 1))
	{
		// successful request 
		/* msg.Format("Response: %s", sc.HTTPResponse.GetChars()); */
		/* sc.AddMessageToLog(msg,1); */
	}
	else
	{
		// problem with the request 
		msg.Format("Error with the Request: %s", sc.HTTPResponse.GetChars());
		sc.AddMessageToLog(msg, 1);
	}
}

std::string s_DebugResponse = "";
void DebugResponse(SCStudyInterfaceRef sc)
{
	SCString msg;
	if (s_DebugResponse != "")
	{
		msg.Format("RESPONSE: %s", s_DebugResponse.c_str());
		sc.AddMessageToLog(msg, 1);

		s_DebugResponse = "";
	}

}
// CURL Callback function to capture the response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::stringstream* s)
{
	size_t totalSize = size * nmemb;
	s->write(static_cast<char*>(contents), totalSize);
	return totalSize;
}

// This function performs a curl synchronous request to the inputted URL
void CURLTelegramPostRequest(const SCString& URL, const std::string& ChatID, const std::string& FilePath, const std::string& caption, SCStudyInterfaceRef sc)
{
	// timeout requests in case of failure to save file 
	auto start = std::chrono::steady_clock::now();
	int maxWaitTimeMillis = 20000;

	// Logic to ensure the file is finished being saved before allowing the request to go through 
	for (;;)
	{
		bool FileSavedSuccessfully = std::filesystem::exists(FilePath);
		if (FileSavedSuccessfully)
		{
			auto FileSize = std::filesystem::file_size(FilePath); // get the file size 
			Sleep(150); // quick pause 
			for (;;)
			{
				if (std::filesystem::file_size(FilePath) > FileSize) // if the current file size is greater than the saved size 
				{
					/* std::cout << "File not finished being written!" << std::endl; */

					// update the file size 
					FileSize = std::filesystem::file_size(FilePath);

					// pause some time 
					Sleep(100);

					// loop again 
					continue;
				}
				else
				{
					// file size was the same. should be finished writing 
					/* std::cout << "File finished being written!" << std::endl; */
					break;
				}
			}
			break; // break outer loop once inner loop breaks 
		}
		else
		{
			/* std::cout << "File not finished being saved!" << std::endl; */
			Sleep(200); // file not saved, wait for it to be saved 

			// if our clock has exceeded the timer, just timeout the request. 
			auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
			if (elapsedMillis > maxWaitTimeMillis)
			{
				// debug 
				/* std::cout << "Request timed out due to file not being saved successfully!" << std::endl; */
				return; // Timeout
			}
		}
	}

	// initialize curl 
	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL* curl = curl_easy_init();

	// Updated to new mime interface: Wed Jul 31 16:54:31 EDT 2024
	curl_mime* mime;
	curl_mimepart* part;

	// debug string 
	std::stringstream responseStream;

	if (curl)
	{
		mime = curl_mime_init(curl);

		// add the chat ID 
		part = curl_mime_addpart(mime);
		curl_mime_data(part, ChatID.c_str(), CURL_ZERO_TERMINATED);
		curl_mime_name(part, "chat_id");

		// handle if there is a group subtopic ID in the Chat ID 
		std::size_t ThreadIDPos = ChatID.find("_");
		if (ThreadIDPos != std::string::npos)
		{
			const std::string MessageThreadID = ChatID.substr(ThreadIDPos + 1);

			// add the message thread id 
			part = curl_mime_addpart(mime);
			curl_mime_data(part, MessageThreadID.c_str(), CURL_ZERO_TERMINATED);
			curl_mime_name(part, "message_thread_id");
		}

		// add the photo 
		part = curl_mime_addpart(mime);
		curl_mime_filedata(part, FilePath.c_str());
		curl_mime_name(part, "photo");

		// add the message caption (text)
		part = curl_mime_addpart(mime);
		curl_mime_data(part, caption.c_str(), CURL_ZERO_TERMINATED);
		curl_mime_name(part, "caption");

		// add the parse mode 
		part = curl_mime_addpart(mime);
		curl_mime_data(part, "HTML", CURL_ZERO_TERMINATED);
		curl_mime_name(part, "parse_mode");

		// debug: gives more curl output from the server 
		/* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

		// Declare headers 
		struct curl_slist* headers = nullptr;

		// Initialize headers as needed
		headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
		/* headers = curl_slist_append(headers, "Expect:");  // Disable 100-continue */

		// set the headers 
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// set it as a mime post request
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

		// Set the URL
		curl_easy_setopt(curl, CURLOPT_URL, URL.GetChars());


		// *********************** Enable verbose logging for debugging *******************
		// **********************                                        *****************
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);


		// Debug  
		// Set the write function
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

		// Pass a pointer to string variable to store the response
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStream);



		// *********************** Set CA certificate path *******************
		// **********************                          *****************

		curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/curl/certs/cacert-2024-07-02.pem");


		// Perform the HTTP POST request
		CURLcode res = curl_easy_perform(curl);

		// debug 
		/* std::cout << "Response code: " << res << std::endl; */
		/* std::cout << "Response: " << responseStream.str() << std::endl; */

		// Check for errors
		if (res != CURLE_OK)
		{
			// debug if necessary 
			/* debug_request = "Curl request failed: " + std::string(curl_easy_strerror(res)); */
			/* msg.Format("CURL Request Error: %s", debug_request.c_str()); */
			/* sc.AddMessageToLog(msg,1); */

			/* s_DebugResponse = "CURL Request Failed: " + responseStream.str() + "\n"; */
			s_DebugResponse = "CURL Request Failed: " + std::string(curl_easy_strerror(res)) + "\n";
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

		}
		else
		{
			s_DebugResponse = "CURL Request Success: " + responseStream.str() + "\n";

			// Log the response
			/* std::cout << "Response code: " << res << std::endl; */
			/* std::cout << "Response: " << responseStream.str() << std::endl; */
		}

		// Clean up
		curl_easy_cleanup(curl);
		curl_mime_free(mime);
		curl_global_cleanup();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
SCSFExport scsf_SV_TelegramDrawingAlert(SCStudyInterfaceRef sc)
{
	// Study Inputs 
	SCInputRef Input_Enabled = sc.Input[0];
	SCInputRef Input_ChatID = sc.Input[1];
	SCInputRef Input_SendChartImage = sc.Input[2];
	SCInputRef Input_ImageFolderPath = sc.Input[3];
	SCInputRef Input_UseCustomBot = sc.Input[4];
	SCInputRef Input_CustomBotToken = sc.Input[5];
	SCInputRef Input_SpecifyPath = sc.Input[6];
	SCInputRef Input_CustomFolderPath = sc.Input[7];
	SCInputRef Input_FindDuplicateStudies = sc.Input[8];
	SCInputRef Input_CustomizeMessageContent = sc.Input[9];
	SCInputRef Input_Debug = sc.Input[10];

	SCString msg; // logging object

	// Declare Necessary persistent variables
	// These are items that need to be remembered between calls to the study function 

	// Keep track of the last modification time of an alert log file. to be
	// This is used to determine if a file has been recently modified.
	std::int64_t& LastModTime = sc.GetPersistentInt64(0);

	// keep track of the number of lines in the file
	// This is in order to only process the most recently added line (only send one alert)
	int& NumberOfLines = sc.GetPersistentInt(4);

	// keep track of the very first search for the file
	// This is in order to handle the behaviour for the very first time Sierra Chart is started 
	int& FirstTimeStart = sc.GetPersistentInt(5);

	if (sc.SetDefaults)
	{
		// This is what appears when you select Description from the study settings
		// If you do not have the study just refer to the README file on the
		// repository which contains the identical documentation. 
		sc.StudyDescription =

			"<strong><u>Info About the Code: </u></strong> "
			"<br> <br> This advanced custom study was written by VerrilloTrading in Q3-Q4 of 2023. This study is open source and it can serve as an example for how to use a Telegram Bot with Sierra Chart. To compile this study users will need to include the necessary dependencies like boost::format and CURL. I tried as much as possible to stick to the Sierra Chart libraries but some things are much more practical with other c++ libraries. For example, there are very few existing examples of HTTP POST functionality using ACSIL. The study provides an example of this in combination with <a href = https://github.com/nlohmann/json target=_blank ref=noopener noreferrer >nlohmann json</a> to prepare the request body. CURL was necessary to perform the <a href = https://core.telegram.org/bots/api#sendphoto target=_blank ref=noopener noreferrer >sendPhoto</a> Telegram method for sending multipart/form data. Anyone is welcome to repurpose the code, improve it or add functionality."

			" <br> <br> This study contains two examples of sending a http post request from Sierra Chart."
			"<br> 1. using CURL to call the Telegram <a href = https://core.telegram.org/bots/api#sendphoto target=_blank ref=noopener noreferrer >sendPhoto</a> method where the chart image is passed as multipart/form data."
			"<br> 2. using <a href = https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scMakeHTTPPOSTRequest target=_blank ref=noopener noreferrer >sc.MakeHTTPPOSTRequest()</a> to call Telegram <a href = https://core.telegram.org/bots/api#sendmessage target=_blank ref=noopener noreferrer >sendMessage</a> method which is a native Sierra Chart ACSIL function."

			"<br> <br> The second example is in the study in the case that the user does not want to recieve the chart screenshot. Otherwise it is meant to serve as an example."

			"<br> <br> Reasons for sharing the source code:"
			"<br> 1. Greater Transparency "
			"<br> 2. Helps the community"
			"<br> 3. Show off skills and features in Sierra Chart"

			"<br> <br> <strong><u>Info for Users: </u></strong> "
			"<br> <br> If you want to use this study and do not want to bother building it yourself you can send an email to <span style=\"background-color: MediumTurquoise; font-weight: bold;\">support@verrillotrading.com</span>"
			" with your Sierra Chart username and I will add your username to the list of authorized users for this DLL. When your username is added to this list the file will appear in your Sierra Chart Data Folder the next time you start Sierra Chart."
			" The DLL has been compiled using this exact source code file **(except one change) and a build script."

			"<br> <br> ** For convenience purposes The DLL provided by VerrilloTrading provides a default bot that users can rely on instead of having to create their own bot. The bot token for that bot was removed from the repository for security reasons."

			"<br> <br> <strong><u>Description of Study Functionality:</u></strong> "

			"<br> <br> When this study is enabled and a <a href = https://www.sierrachart.com/index.php?page=doc/Tools.html#ChartDrawingAlerts target=_blank rel=noopener noreferrer >Sierra Chart Drawing Alert</a> or <a href = https://www.sierrachart.com/index.php?page=doc/StudyChartAlertsAndScanning.php target=_blank rel=noopener noreferrer >Chart/Study Alert</a> takes place, a Telegram message including a chart screenshot will be sent to the specified Telegram Chat ID. This message will contain information about the alert that took place. For a full demonstration please watch this accompanying <a href = https://youtu.be/EQZI9pBtDrE target=_blank rel=noopener noreferrer >video</a> on YouTube, and continue to follow along with this document. "

			" This study handles different cases and exceptions for when Sierra Chart generates new Alert Log Files. If you find any undefined behaviour or a replicable bug, please inform the study developer at the above email."


			"<br> <br> <strong><u>HOW TO SETUP AND USE THIS STUDY:</u></strong> "
			"<br> <br> <strong><u>Step 1:</u></strong> "
			"<br> <br> It is necessary to be running Sierra Chart Version <strong>2644</strong> or higher to use this study. "
			"<br> <br> For this study to work it is necessary to <u>Enable</u> this setting in Sierra Chart:"
			"<br> <span style=\"background-color: yellow; font-weight: bold;\">Global Settings >> Log >> Save Alerts Log to File</span>"

			"<br> <br> When an Alert is triggered, a new line is written to a file in the <strong>C:\\SierraChart\\Logs</strong> directory. The path to this Folder is relative to where you have Sierra Chart installed. It is also possible that this Logs Folder does not exist yet. <span style=\"background-color: yellow; text-decoration: underline;\">Follow the next steps to ensure this folder exists.</span> "

			"<br> <br> When putting the study on the chart for the first time it will provide instructions in the Message Log. The first message will inform the user to enter the Telegram Chat ID."


			"<br> <br> <strong><u>Step 2:</u></strong> "
			"<br> <br> Get the Telegram Chat ID of where we want to send the alert. The easiest way is to use Telegram Web to get the Telegram Chat ID. Telegram Groups, Group Subtopics, Channels and Private Chats are supported. <span style=\"background-color: yellow; font-weight: bold; \">It is necessary to be on Telegram Web version A to get the Chat ID from the browser address bar.</span> The version can be selected from the settings window in the top left corner when using Telegram Web. The Chat ID for the currently open chat appears in the address bar. Try changing to different chats to see the end of the address bar change. Example: https://web.telegram.org/a/#-12345678 where '-12345567' is the Chat ID. Groups and Group Sub-Topics typically have negative numbers as the Chat ID. To get your personal chat ID, go to the Saved Messages chat. On Telegram Web this can be accessed by clicking at the top left and selecting 'Saved Messages'. The address bar should display your Private Chat ID. In order to the send messages to a Telegram Channel it is necessary to give the bot administrator permissions in the channel, specifically with the permission to send messages. "

			" <br> <br> The default bot that the study uses on Telegram is <a href = https://t.me/vt_sierrachart_sender_bot target=_blank rel=noopener noreferrer >https://t.me/vt_sierrachart_sender_bot.</a> "
			"<br> You need to message this bot with /start on Telegram or add it to your group, unless you are using your own bot. Telegram bots cannot message you first and they cannot read messages sent by other bots. "

			"<br> <br> Users can override the default bot with their their own bot if they prefer. "

			"<br> <br> <strong><u>Step 3:</u></strong> "

			"<br> <br> Perform the following steps after you have inputted the Telegram Chat ID in Step 2. If you still see an Error in the Message Log about not being able to find the Logs folder, perform the following steps: "

			"<br> <br> 1. Make sure you enabled the setting mentioned above named <a href = https://www.sierrachart.com/index.php?page=doc/GeneralSettings.html#SaveAlertsLogToFile target=_blank rel=noopener noreferrer >Save Alerts Log to File. </a>"
			"<br> 2. Check if the Folder 'Logs' exists in your Sierra Chart installation directory."
			"<br> 3. If the Logs folder does not exist, create a new <a href = https://www.sierrachart.com/index.php?page=doc/Tools.html#ChartDrawingAlerts target=_blank rel=noopener noreferrer >Chart Drawing Alert</a> and make sure it is triggered. Once this first alert is triggered, the folder should be created. "
			"<br> 4. Check inside the Logs folder for a file that starts with 'Alert Log' followed by the current DateTime. If you do not see this file, repeat the instructions in Step 1. "

			"<br> <br> <strong><u>Step 4:</u></strong> "

			"<br> <br> <strong>Important:</strong> Make sure Study Input #4 is set to the Image Folder Path that is set in <strong>Global Settings > Paths > Chart Image Folder.</strong> When this setting is changed in Sierra Chart, a new folder will be automatically created. It is absolutely critical that you set Study Input #4 input text to this path, or the screenshot will not work. You will get an Error showing up in the message log if the path you specified in this Study Input does not exist. The default path for this on Windows is: <strong>C:\\SierraChart\\Images</strong> If you are running on Linux through Wine or if you changed the drive letter of your C drive it might look like this: <strong>Z:\\mnt\\SierraChart\\Images</strong>. This study is confirmed to work when Sierra Chart is being run on Mac using Parallels."

			"<br> <br> For the screenshot function to work when charts are hidden behind other charts in your chartbook it is necessary to <span style=\"background-color: yellow; font-weight: bold;\">disable</span> <a href = https://www.sierrachart.com/index.php?page=doc/GeneralSettings.html#DestroyChartWindowsWhenHidden target=_blank rel=noopener noreferrer >Destroy Chart Windows When Hidden</a>."

			"<br> <br> <u><strong>STUDY AND ALERT CONFIGURATION:</strong></u> "

			"<br> <br> <u><strong>Alert Settings:</strong></u> "

			"<br> <br> Users have multiple options for how they can configure Alerts in Sierra Chart. A Chart Alert is set up by going to Chart Settings > Alert. Identical settings exist for Study Alerts which can be set up in the Study Settings for any particular study and going to the Alerts tab. To enable a Chart/Study Alert, an alert formula is necessary. Refer to <a = href https://www.sierrachart.com/index.php?page=doc/StudyChartAlertsAndScanning.php target=_blank rel=noopener noreferrer >this documentation</a> for info about Chart/Study Alerts and Formula examples. If the user wants to be repeatedly notified of an alert being triggered, they should disable <strong><i>Disable Simple Alert After Trigger</i></strong>. This will result in repeated alerts being sent to Telegram, about one second apart. If the user wants to send one alert and disable the alert when it is triggered, it is necessary to enable this setting."

			"<br> <br> <i>Chart Drawing Alerts</i> are configured by right clicking on an existing Chart Drawing and selecting <i>Alerts</i>. Similar settings exist for <i>Chart Drawing Alerts</i> where the user can choose to enable <i>Only Trigger Once</i> or <i>Alert Once Per Bar</i>."

			"<br> <br> <u><strong>Usage Across Multiple Chartbooks:</strong></u> "
			"<br> <br> <a href = https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scSaveChartImageToFileExtended target=_blank rel=noopener noreferrer >The screenshot function from Sierra Chart</a> is limited to taking screenshots of charts in the chartbook where the function call originates from. For this reason <strong>it is necessary to have 1 instance of this study open per chartbook</strong>. If you have 5 open chartbooks with different symbols waiting for alerts, you will need to open this study on a chart in each one of those chartbooks. When the study is active in any particular chartbook it will handle all the alerts that take place within that chartbook and provide an image of the chart as it is displayed in the chartbook. The chart can be hidden behind other charts and the screenshot will work as long as <a href = https://www.sierrachart.com/index.php?page=doc/GeneralSettings.html#DestroyChartWindowsWhenHidden target=_blank rel=noopener noreferrer >Destroy Chart Windows When Hidden</a> is <span style=\"background-color: yellow; font-weight: bold;\">disabled</span>. "

			"<br> <br> <strong><u>Using Duplicates of This Study Within the Same Chartbook:</u></strong> "
			"<br> <br> One viable use case for having more than one instance of this study open in the same chartbook is to send the same alerts to multiple Telegram chats at the same time. In this case it is necessary to configure the each study with valid settings and use a different Telegram Chat ID for each instance of the study. If the same Chat ID is used on two or more of these studies, the user will get duplicate alerts sent to the same chat. "

			"<br> <br> <strong><u>Detecting Duplicates of This Study Within the Same Chartbook:</u></strong> "
			"<br> <br> If the user added the study more than once by accident, they can enable a feature that provides the user with the chart numbers of any chart containing the Telegram Chart & Drawing Alerts study within the same chartbook. "

			"<br> <br> 1. Enable Study Input #9 which is named <u><i>Find Duplicate Studies in Current Chartbook</i></u>."
			"<br> 2. Open the Sierra Chart Message Log by going to Window > Message Log."
			"<br> 3. Create and trigger a Chart Drawing or Chart Alert and monitor the Message Log."
			"<br> 4. Look out for a message that looks like this: "
			"<br> <br> \"A duplicate is found on chart #(chart number here). Reduce the number of studies per chartbook to one unless you wish to recieve duplicate alerts.\"  "

			"<br> <br> 5. Then you can proceed to Window > Windows and Chartbooks, select the chart that has this chart number and remove the study from this chart."

			"<br> <br> <strong><u>How to tell if Alerts are not going through to Telegram:</u></strong> "

			"<br> <br> It is fully possible that users are able to find a bug in the study where an alert is not being recieved on Telegram. One reason for this might be a particular text that is not in agreement with the Telegram Text formatting being used. "

			"<br> <br> 1. Open the SC Alert Manager by going to Window > Alert Manager."
			"<br> <br> 2. Monitor the alerts being added to the Alert Manager."
			"<br> <br> 3. If you are encountering a particular alert that is not going through to Telegram, please inform the study developer at support@verrillotrading.com as soon as possible. "

			"<br> <br> <strong><u>Known or potential Issues :</u></strong> "

			"<br> <br> There are some settings in Sierra Chart that can change the formatting for text that is saved to the Alert Log files. "
			"<br> Here are settings that are known to cause issues with the study: "

			"<br> <br> 1. General Settings > Charts > Show Chart Number First on Chart Name "
			"<br> This setting should remain set to <strong><u>no</u></strong>. "

			"<br> <br> <strong><u>INFO ABOUT THE LOG FILE PATH:</u></strong> "

			"<br> <br> The following information applies to you if you have moved the Sierra Chart Data Files Folder out of the SC Installation directory. "


			"<br> <br> <strong>It is not recommended to move the location of the <a href = https://www.sierrachart.com/index.php?page=doc/GeneralSettings.html#DataFilesFolder target=_blank rel=noopener referrer >Data Files Folder</a> from the Sierra Chart installation directory.</strong> "

			"<br> <br> If you absolutely need to move this folder or have already moved it, you will need to perform this next step: "
			"<br> <br> <strong>IMPORTANT:</strong> If you have changed the location of the Data Files Folder, You will need to explicitly specify the directory of the Sierra Chart Logs folder. This folder will be located in the same directory where Sierra Chart is installed. The reason why this is necessary is because the study uses the Data Files Folder Path to get the Path to the Logs Folder. Example: if you moved the Data Folder to ANY PATH that is not the Sierra Chart Installation directory, you will now need to specify the Path of the Logs Folder which is in the directory where Sierra Chart is installed. <span style=\"background-color: yellow; text-decoration: underline;\">You need to explicitly specify the Logs Folder path in the last study input field.</span> "

			"<br> <br> The default location of this folder is: <strong>C:\\SierraChart\\Logs</strong> If you are running on Linux through Wine or if you changed the drive letter of your C drive it might look like this: <strong>Z:\\mnt\\SierraChart\\Logs</strong>"

			"<br> <br> For any issues or inquiries, send them to support@verrillotrading.com"

			"<br> <br> Thank you and enjoy the study!"
			"<br> <br> -VerrilloTrading, Content Creator - Programmer";

		// Study Defaults
		sc.GraphName = "Sua Telegram Chart & Drawing Alerts";
		sc.AutoLoop = 0;//Manual looping
		sc.GraphRegion = 0;
		sc.ScaleRangeType = SCALE_SAMEASREGION;

		sc.UpdateAlways = 1; // not strictly necessary but can make it faster 

		Input_Enabled.Name = "Send Telegram on Alert Trigger";
		Input_Enabled.SetYesNo(1);
		Input_Enabled.SetDescription("Allows the user to turn the study on or off.");

		Input_ChatID.Name = "Telegram Chat ID to Send Message";
		Input_ChatID.SetString("-4277453820");
		Input_ChatID.SetDescription("User must provide the Telegram Chat ID for the Bot to send a message to.");

		Input_SendChartImage.Name = "Send Chart Screenshot in Telegram Message";
		Input_SendChartImage.SetYesNo(1);
		Input_SendChartImage.SetDescription("Allows the user to control if a chart screenshot is sent along with the message.");

		Input_ImageFolderPath.Name = "Full Path to Chart Image Folder";
		Input_ImageFolderPath.SetString("C:\\SierraChart\\Images");
		Input_ImageFolderPath.SetDescription("User must put in the exact path to the Chart Image Folder. "
			"It must match what is set in Global Settings > General Settings > Paths > Chart Image Folder (Action).");

		Input_UseCustomBot.Name = "Use a Different Telegram Bot";
		Input_UseCustomBot.SetYesNo(1);
		Input_UseCustomBot.SetDescription("Allows the user to specify the use of a different Telegram Bot.");

		Input_CustomBotToken.Name = "Telegram Bot Token for Use Different Bot";
		Input_CustomBotToken.SetString("7324959755:AAF0Ruo-L2_2mXDnK8wXbS0QIvCN9QPiovA");
		Input_CustomBotToken.SetDescription("If Previous Input Enabled: User must specify the Bot API token for the Bot. Bot API Tokens can be obtained by chatting with Bot Father on Telegram.");

		Input_SpecifyPath.Name = "Special case: SC Data Folder is not in Installation Directory";
		Input_SpecifyPath.SetYesNo(0);
		Input_SpecifyPath.SetDescription("Allows the user to specify if they have moved the Data Files Folder out of the Sierra Chart Installation Directory.");

		Input_CustomFolderPath.Name = "Special case: Explicit Path to Logs Directory";
		Input_CustomFolderPath.SetString("C:\\SierraChart\\Logs");
		Input_CustomFolderPath.SetDescription("If Previous Input Enabled: User must put in the exact path to the Logs Folder. This folder is found in the Sierra Chart Installation Directory. Example: if Sierra Chart is installed to Z:\\mnt\\SierraChart23\\ you would set this input to be: Z:\\mnt\\SierraChart23\\Logs");

		Input_FindDuplicateStudies.Name = "Find Duplicate Studies in Current Chartbook";
		Input_FindDuplicateStudies.SetYesNo(0);
		Input_FindDuplicateStudies.SetDescription("This input can be enabled to determine if there are duplicate instances of this study within the same chartbook. When set to yes and two or more instances of this study exist within the same chartbook, a message will be added to the Sierra Chart message log containing the chart number where the duplicate study is found.");

		Input_CustomizeMessageContent.Name = "Remove Items from Message Text for Chart/Study Alerts";
		Input_CustomizeMessageContent.SetCustomInputStrings("Off;Remove Formula;Remove All Except Study Name");
		Input_CustomizeMessageContent.SetCustomInputIndex(0);
		Input_CustomizeMessageContent.SetDescription("This setting allows the user to remove the alert formula, or only keep the Study Name in the Telegram message text for Chart/Study Alerts.");

		Input_Debug.Name = "Print CURL Request Response in Message Log";
		Input_Debug.SetYesNo(1);
		Input_Debug.SetDescription("This input is used to debug a http request by printing the reseponse to the SC Message log.");

		// Persistent variable precaution 
		if (FirstTimeStart != 0)
		{
			FirstTimeStart = 0;
		}

		return;

	}
	// End Study Defaults 

	/*/if (sc.IsUserAllowedForSCDLLName == false)
	{
		if (sc.Index == 0)
		{
			sc.AddMessageToLog("User is not authorized to use this study", 1);
		}
		return;
	}*/


	// Add support for Enabling and Disabling the study via the input
	if (Input_Enabled.GetBoolean() == 0)
	{
		// debug 
		// sc.AddMessageToLog("Error: Study is disabled.",1);

		return; // the study is turned off
	}

	// Thu Jul 18 16:49:35 EDT 2024
	// debug 
	if (Input_Debug.GetBoolean() == 1)
	{
		DebugResponse(sc);
	}

	// Check for the Chat ID. 
	if (strlen(Input_ChatID.GetString()) == 0)
	{
		sc.AddMessageToLog("Error: You must specify a Telegram Chat ID.", 1);
		return;
	}

	// variable for telegram bot api token
	SCString token = "";

	// If they want to use a custom bot, make sure they also filled in the token
	if (Input_UseCustomBot.GetBoolean() == 1)
	{
		// debug 
		/* sc.AddMessageToLog("User has requested to use a custom bot", 1); */

		if (strlen(Input_CustomBotToken.GetString()) == 0) // Do not proceed if there is no Bot Token. 
		{
			msg.Format("Error: User Specified Custom Bot Token but Custom Bot API Token is not set.");
			sc.AddMessageToLog(msg, 1);
			return;
		}
		else
		{
			// get the bot token from SC Input. Concatenate a plain string with SC String. 
			token = SCString("bot") + Input_CustomBotToken.GetString();

			// debug 
			/* msg.Format("User Specified Bot Token: %s", token.GetChars()); */
			/* sc.AddMessageToLog(msg,1); */
		}
	}
	else
	{
		// User wants to use the default bot. On the github code there is no default bot so the study will just bomb out.
		sc.AddMessageToLog("If you see this message it means you compiled the study on your own. "
			" It is necessary for you to use your own bot using the provided study input field.", 1);
		return;

		token = SCString("bot") + "your_bot_token_here";

	}

	// get the Telegram Chat ID from SC Input. 
	std::string ChatID = Input_ChatID.GetString();

	// declare variables for the host and Telegram method being called
	SCString host = "https://api.telegram.org/";

	// two variables that will be changed later by the parsing function
	SCString sc_alert_chartbook = "";
	int sc_alert_chart_num = 0;

	// Get the current date time (used for formatting file name object)
	SCDateTime CurrentDateTime;
	CurrentDateTime = sc.CurrentSystemDateTime;

	// variable used to format image file naming 
	std::string SourceChartImageText = "";

	// Variable for Logs Folder Path
	std::string LogsFolderPath = "";

	// Confirm if the user specified the path in study input or not
	if (Input_SpecifyPath.GetBoolean() == 1)
	{
		// Catch if they turned it on but did not specify the path.  
		if (strlen(Input_CustomFolderPath.GetString()) == 0)
		{
			sc.AddMessageToLog("Error: User has specified Use Custom Path but no path was provided", 1);
		}
		else
		{
			// set the explicitly specified path 
			LogsFolderPath = Input_CustomFolderPath.GetString();
		}
	}
	else
	{
		// Get the Folder Path using the SC Data Files Folder Path
		GetLogsFolderPath(sc, LogsFolderPath);
		if (LogsFolderPath == "Invalid_Path") // if it is invalid bomb out the study function
		{
			return;
		}
	}

	// Variable to save the file write time in seconds to be compared against
	// the file write time in seconds in memory 
	int64_t Local_LastModTime = 0;

	// variable to save the file name 
	std::string most_recent_filename;

	// count number of files to check against number of files in memory 
	int number_of_files = 0;

	// Persistently REMEMBER the number of files 
	// This is set explicitly in two places:
	// 1. When the study iterates through the directory in the following code block 
	// 2. at the very end of the study function. Which is necessary for when there are no log files to begin
	// And a new log file has been created. 
	int& num_files_memory = sc.GetPersistentInt(10);

	// FIND THE MOST RECENT FILE 
	FindMostRecentFile(sc, LogsFolderPath, number_of_files, Local_LastModTime, most_recent_filename);

	// debug to check if we are getting the right file
	/* msg.Format("There are %d total files | Most recent filename: %s" */
	/* ,number_of_files, most_recent_filename.c_str()); */
	/* sc.AddMessageToLog(msg,1); */

	// Bomb out if there are no alert log files 
	if (number_of_files == 0)
	{
		/* sc.AddMessageToLog("there are no alert files found",1); */
		return; // bomb out until an alert file is found
	}

	// ENSURE on the first call to the function that number of files is initialized to MEMORY
	// NECESSARY for the check below that determines if a new log file was created.
	if (num_files_memory == 0 && number_of_files != 0)
	{
		// we should only get here if there are alert log files and number of files in
		// memory has not been initialized yet. 

		// REMEMBER the number of alert log files.
		num_files_memory = number_of_files;
	}

	// Construct the full path by combining the directory path and filename
	// c++ filesystem variable which is explicitly set to the name of the file
	// with the most recent write time in the specified directory. 
	std::filesystem::path LogFile(LogsFolderPath + "\\" + most_recent_filename);
	if (std::filesystem::exists(LogFile))
	{
		// save the filename as a string 
		std::string filename = LogFile.filename().string();

		// debug 
		/* msg.Format("Log File Exists...Filename match: %s", filename.c_str()); */
		/* sc.AddMessageToLog(msg,1); */

		// SAVE the last write time of our file	
		std::filesystem::file_time_type currentModTime = std::filesystem::last_write_time(LogFile);

		// CONVERT it to time in seconds from epoch
		auto currentModTimePoint = std::chrono::duration_cast<std::chrono::seconds>
			(currentModTime.time_since_epoch()).count();

		if (num_files_memory != number_of_files)
		{
			// If we get here it means there has been a change
			// to the number of files in the Logs Directory 
			//
			// Case 1: Sierra Chart just created a new log file after being
			// opened, and we need to read the first line out of that new file
			// and send a Telegram.
			//
			// This handles the case for when SC is opened and the study is
			// already on the chart and SC creates a brand new alert log file
			// as soon as an alert is triggered.
			//
			// Case 2: We may also get here if a file is deleted or added to
			// the directory This is being handled on the following lines
			//
			//
			// bug fix 2023-11-20
			// This handles if a file gets removed from the logs folder while
			// the study is active because this is one case where number of
			// files in memory could be larger than number of files. 
			if (num_files_memory > number_of_files)
			{
				num_files_memory = number_of_files;
				return;
			}

			// SAVE THE last write time of our file TO MEMORY
			LastModTime = currentModTimePoint;

			// First read how many lines are in the file
			// open the file for reading
			std::ifstream file(LogFile);
			if (file)
			{
				// Save the line text for formatting 
				std::string line;
				int CountLines = 0; // count the number of lines
				while (std::getline(file, line))
				{
					CountLines++;

					// REMEMBER globally the new number of lines 
					NumberOfLines = CountLines;

					// Process the line and send a Telegram

					// Handles text formatting for Chart/Study Alerts
					if (line.find("Type: Study") != std::string::npos)
					{
						// Get the user setting for customize message content
						int CustomizeMessageContent = Input_CustomizeMessageContent.GetIndex();

						// Parse the alert log file and prepare the text for the request 
						// Also return the chartbook name that generated the alert 
						ParseChartStudyAlertText(line, SourceChartImageText, CustomizeMessageContent,
							sc_alert_chartbook, sc_alert_chart_num);

						// Get the name of the chartbook where this study is applied 
						SCString current_chartbook = sc.ChartbookName();

						// The alert came from the same chartbook as where this study exists
						// Each chartbook must handle it's own chart screenshots
						if (sc_alert_chartbook == current_chartbook)
						{
							// Feature: FIND DUPLICATE STUDIES IN THE SAME CHARTBOOK, ENABLED WITH A STUDY INPUT
							if (Input_FindDuplicateStudies.GetYesNo())
							{
								// Call the function which simply returns a message in the log if a duplicate is found
								FindDuplicateStudiesInSameChartbook(sc, sc.GraphName.GetChars(), msg);
							}

							// Confirm which Telegram method the user specified
							if (Input_SendChartImage.GetBoolean() == 0)
							{
								// use sendMessage method instead of sendPhoto
								SCTelegramPostRequest(sc, host, token, ChatID, line, msg);
							}
							else
							{
								// set the method variable which is passed into the URL string
								SCString method = "/sendPhoto?";

								// Set our Telegram URL for the POST request 
								SCString URL = std::move(host) + std::move(token) + std::move(method);

								// get the year month and day from SCDateTimeVariable
								int Year, Month, Day, Hour, Minute, Second;
								CurrentDateTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);

								// Format the text for the image file name 
								// The source string combined with the current date time 
								msg.Format("\\%s %d-%d-%d %d_%d_%d.png", SourceChartImageText.c_str(), Year, Month, Day,
									Hour, Minute, Second);

								// Create the file path to our image file 
								std::string FilePath = Input_ImageFolderPath.GetString()
									+ std::string(msg.GetChars());

								// HANDLE if the folder does not exist before accessing the directory 
								if (!std::filesystem::exists(Input_ImageFolderPath.GetString()) &&
									!std::filesystem::is_directory(Input_ImageFolderPath.GetString()))
								{
									sc.AddMessageToLog("Error: Specified Image Folder Does not Exist! "
										"There may be an error with the request ", 1);
								}

								// convert string into SC String to pass into SC screenshot function 
								SCString SC_FilePathName = FilePath.c_str();

								// Take a screen shot 
								sc.SaveChartImageToFileExtended(sc_alert_chart_num, SC_FilePathName, 0, 0, 0);

								// Call the HTTP POST Request in a separate thread. (not recommended for large scale operations)
								// This function calls sendPhoto Telegram method
								std::thread request_thread(CURLTelegramPostRequest, URL, ChatID, FilePath, line, std::ref(sc));

								// detach the thread and it will finish on it's own. 
								request_thread.detach();
							}
						}
						else
						{
							// the alert came from a different chartboook
							// Simply do nothing and let the study function finish 

							/* sc.AddMessageToLog("Alert was generated from a different chartbook. " */
							/* "That chartbook will handle the alert.",1); */
						}
					}
					else if (line.find("Type: Drawing") != std::string::npos)
					{
						// Get the user setting for customize message content
						int CustomizeMessageContent = Input_CustomizeMessageContent.GetIndex();

						// Parse the alert log file and prepare the text for the request 
						// Also return the chartbook name that generated the alert 
						ParseChartDrawingAlertText(line, SourceChartImageText, CustomizeMessageContent,
							sc_alert_chartbook, sc_alert_chart_num);

						// Get the name of the chartbook where this study is applied 
						SCString current_chartbook = sc.ChartbookName();

						// the alert came from the same chartbook as where this study exists
						if (sc_alert_chartbook == current_chartbook)
						{
							// FIND DUPLICATE STUDIES IN THE SAME CHARTBOOK, ENABLED WITH A STUDY INPUT
							if (Input_FindDuplicateStudies.GetYesNo())
							{
								// Call the function which simply returns a message in the log if a duplicate is found
								FindDuplicateStudiesInSameChartbook(sc, sc.GraphName.GetChars(), msg);
							}

							// SEND TELEGRAM message or message with photo 
							// Confirm which Telegram method the user specified
							if (Input_SendChartImage.GetBoolean() == 0)
							{
								// use sendMessage method instead of sendPhoto
								SCTelegramPostRequest(sc, host, token, ChatID, line, msg);
							}
							else
							{
								// set the method variable which is passed into the URL string
								SCString method = "/sendPhoto?";

								// get the year month and day from SCDateTimeVariable
								int Year, Month, Day, Hour, Minute, Second;
								CurrentDateTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);

								// Format the text for the image file name 
								// The source string combined with the current date time 
								msg.Format("\\%s %d-%d-%d %d_%d_%d.png", SourceChartImageText.c_str(), Year, Month, Day,
									Hour, Minute, Second);

								// Create the file path to our image file 
								std::string FilePath = Input_ImageFolderPath.GetString()
									+ std::string(msg.GetChars());

								// NOTIFY if the folder does not exist before accessing the directory 
								if (!std::filesystem::exists(Input_ImageFolderPath.GetString()) &&
									!std::filesystem::is_directory(Input_ImageFolderPath.GetString()))
								{
									sc.AddMessageToLog("Error: Specified Image Folder Does not Exist! "
										"There may be an error with the request ", 1);
								}

								// convert/move string into SC String to pass into SC function 
								SCString SC_FilePathName = FilePath.c_str();

								// Take a screen shot and put it in our logs folder directory for safety 
								sc.SaveChartImageToFileExtended(sc_alert_chart_num, SC_FilePathName, 0, 0, 0);

								// Set our Telegram URL for the POST request 
								SCString URL = std::move(host) + std::move(token) + std::move(method);

								// Call the HTTP POST Request in a separate thread. (not recommended for large scale operations)
								// This function calls sendPhoto Telegram method
								std::thread request_thread(CURLTelegramPostRequest, URL, ChatID, FilePath, line, std::ref(sc));

								// detach the thread and it will finish on it's own. 
								request_thread.detach();
							}
						}
						else
						{
							// the alert came from a different chartboook
							// Simply do nothing 
							//
							/* sc.AddMessageToLog("Alert was generated from a different chartbook. " */
							/* "That chartbook will handle the alert.",1); */
						}
					}
				}
				// close the alert log file (Outer most file) 
				file.close();
			}
			else
			{
				sc.AddMessageToLog("Error opening file!#@", 1);
			}
		}

		// FIRST TIME READ FILE
		if (FirstTimeStart == 0)
		{
			// We should get here if the study has just been initialized on the chart aka 
			// Sierra Chart has just been opened. 
			//
			// The first time the study reads the directory no http requests are sent. 

			// SAVE THE last write time of our file TO MEMORY
			LastModTime = currentModTimePoint;

			FirstTimeStart = 1; // set this variable to 1 in order to not trigger again

			// First read how many lines are in the file

			// open the file for reading
			std::ifstream file(LogFile);
			if (file)
			{
				// count the number of lines in the file 
				std::string line;
				int CountLines = 0;
				while (std::getline(file, line))
				{
					CountLines++;
				}

				// Remember globally the number of lines
				NumberOfLines = CountLines;

				// close the file 
				file.close();
			}
			else
			{
				sc.AddMessageToLog("Could not open file!!!", 1);
			}

		}
		else // EVERY SUBSEQUENT TIME WE READ THE FILE AFTER SC HAS CREATED A NEW ALERT LOG FILE WE GET HERE 
		{
			// Poll the file for a change to the last write time. (time in seconds from epoch)
			if (currentModTimePoint > LastModTime)
			{
				// We get here if write time is different than what was previously saved

				// This means there was an update to the file 
				// and we need to process the update that was made to the file

				// update the last modified time of the file in memory
				LastModTime = currentModTimePoint;

				// open the file for reading (precautionary first check)
				std::ifstream file(LogFile);
				if (file)
				{
					// Variable used to get the line and also format the line to output 
					std::string line;

					// Precaution: if number of lines has not been initialized we need to read it first to figure it out
					if (NumberOfLines == 0)
					{
						int CountLines = 0; // count the number of lines
						while (std::getline(file, line))
						{
							CountLines++;
						}
						NumberOfLines = CountLines;
					}

					// Precaution/good practice: close the file after first read because the next command opens it again
					file.close();

					// Open the file for reading 
					std::ifstream file(LogFile);
					if (file)
					{
						int CountLines = 0; // count the number of lines
						while (std::getline(file, line))
						{
							CountLines++;

							// Check if line counter is greater than previously
							// saved number of lines (only process new lines)
							if (CountLines > NumberOfLines)
							{
								// REMEMBER globally the new number of lines 
								NumberOfLines = CountLines;

								// Check for if its a Chart/Study Alert or Chart Drawing Alert 
								if (line.find("Type: Study") != std::string::npos)
								{
									// Get the user setting for customize message content
									int CustomizeMessageContent = Input_CustomizeMessageContent.GetIndex();

									// Parse the alert log file and prepare the text for the request 
									// Also return the chartbook name that generated the alert 
									ParseChartStudyAlertText(line, SourceChartImageText, CustomizeMessageContent,
										sc_alert_chartbook, sc_alert_chart_num);

									// Get the name of the chartbook where this study is applied 
									SCString current_chartbook = sc.ChartbookName();

									// The alert came from the same chartbook as where this study exists
									// Each chartbook must handle it's own chart screenshots
									if (sc_alert_chartbook == current_chartbook)
									{
										// Feature: FIND DUPLICATE STUDIES IN THE SAME CHARTBOOK, ENABLED WITH A STUDY INPUT
										if (Input_FindDuplicateStudies.GetYesNo())
										{
											// Call the function which simply returns a message in the log if a duplicate is found
											FindDuplicateStudiesInSameChartbook(sc, sc.GraphName.GetChars(), msg);
										}

										// Confirm which Telegram method the user specified
										if (Input_SendChartImage.GetBoolean() == 0)
										{
											// use sendMessage method instead of sendPhoto
											SCTelegramPostRequest(sc, host, token, ChatID, line, msg);
										}
										else
										{
											// set the method variable which is passed into the URL string
											SCString method = "/sendPhoto?";

											// Set our Telegram URL for the POST request 
											SCString URL = std::move(host) + std::move(token) + std::move(method);

											// get the year month and day from SCDateTimeVariable
											int Year, Month, Day, Hour, Minute, Second;
											CurrentDateTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);

											// Format the text for the image file name 
											// The source string combined with the current date time 
											msg.Format("\\%s %d-%d-%d %d_%d_%d.png", SourceChartImageText.c_str(), Year, Month, Day,
												Hour, Minute, Second);

											// Create the file path to our image file 
											std::string FilePath = Input_ImageFolderPath.GetString()
												+ std::string(msg.GetChars());

											// HANDLE if the folder does not exist before accessing the directory 
											if (!std::filesystem::exists(Input_ImageFolderPath.GetString()) &&
												!std::filesystem::is_directory(Input_ImageFolderPath.GetString()))
											{
												sc.AddMessageToLog("Error: Specified Image Folder Does not Exist! "
													"There may be an error with the request ", 1);
											}

											// convert string into SC String to pass into SC screenshot function 
											SCString SC_FilePathName = FilePath.c_str();

											// Take a screen shot and put it in our logs folder directory for safety 
											sc.SaveChartImageToFileExtended(sc_alert_chart_num, SC_FilePathName, 0, 0, 0);

											// Call the HTTP POST Request in a separate thread. 
											// (not recommended for large scale operations)
											// This function calls sendPhoto Telegram method
											std::thread request_thread(CURLTelegramPostRequest, URL, ChatID, FilePath, line, std::ref(sc));

											// detach the thread and it will finish on it's own. 
											request_thread.detach();
										}
									}
									else
									{
										// the alert came from a different chartboook
										// Simply do nothing and let the study function finish 
										// Important: 1 instance of this study will be necessary per chartbook 
										// in order to handle taking screenshots of those charts in the other chartbooks 

										/* sc.AddMessageToLog("Alert was generated from a different chartbook. " */
										/* "That chartbook will handle the alert.",1); */
									}

								}
								else if (line.find("Type: Drawing") != std::string::npos)
								{
									// Get the user setting for customize message content
									int CustomizeMessageContent = Input_CustomizeMessageContent.GetIndex();

									// Parse the alert log file and prepare the text for the request 
									// Also return the chartbook name that generated the alert 
									ParseChartDrawingAlertText(line, SourceChartImageText, CustomizeMessageContent,
										sc_alert_chartbook, sc_alert_chart_num);

									// Get the name of the chartbook where this study is applied 
									SCString current_chartbook = sc.ChartbookName();

									// the alert came from the same chartbook as where this study exists
									if (sc_alert_chartbook == current_chartbook)
									{
										// FIND DUPLICATE STUDIES IN THE SAME CHARTBOOK, ENABLED WITH A STUDY INPUT
										if (Input_FindDuplicateStudies.GetYesNo())
										{
											// Call the function which simply returns a message in the log if a duplicate is found
											FindDuplicateStudiesInSameChartbook(sc, sc.GraphName.GetChars(), msg);
										}


										if (Input_SendChartImage.GetBoolean() == 0)
										{
											// use sendMessage method instead of sendPhoto
											SCTelegramPostRequest(sc, host, token, ChatID, line, msg);
										}
										else
										{
											// set the method variable which is passed into the URL string
											SCString method = "/sendPhoto?";

											// Set our Telegram URL for the POST request 
											SCString URL = std::move(host) + std::move(token) + std::move(method);

											// get the year month and day from SCDateTimeVariable
											int Year, Month, Day, Hour, Minute, Second;
											CurrentDateTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);

											// Format the text for the image file name 
											// The source string combined with the current date time 
											msg.Format("\\%s %d-%d-%d %d_%d_%d.png", SourceChartImageText.c_str(), Year, Month, Day,
												Hour, Minute, Second);

											// debug image text
											/* sc.AddMessageToLog(msg,1); */

											// Create the file path to our image file 
											std::string FilePath = Input_ImageFolderPath.GetString()
												+ std::string(msg.GetChars());

											// HANDLE if the folder does not exist before accessing the directory 
											if (!std::filesystem::exists(Input_ImageFolderPath.GetString()) &&
												!std::filesystem::is_directory(Input_ImageFolderPath.GetString()))
											{
												sc.AddMessageToLog("Error: Specified Image Folder Does not Exist! "
													"There may be an error with the request ", 1);
											}

											// convert string into SC String to pass into SC screenshot function 
											SCString SC_FilePathName = FilePath.c_str();

											// Take a screen shot and put it in our logs folder directory for safety 
											sc.SaveChartImageToFileExtended(sc_alert_chart_num, SC_FilePathName, 0, 0, 0);

											// Call the HTTP POST Request in a separate thread. 
											// (not recommended for large scale operations)
											// This function calls sendPhoto Telegram method
											std::thread request_thread(CURLTelegramPostRequest, URL, ChatID, FilePath, line, std::ref(sc));

											// detach the thread and it will finish on it's own. 
											request_thread.detach();
										}
									}
									else
									{
										// the alert came from a different chartboook
										// Simply do nothing and let the study function finish 
										// Important: 1 instance of this study will be necessary per chartbook 
										// in order to handle taking screenshots of those charts in the other chartbooks 

										/* sc.AddMessageToLog("Alert was generated from a different chartbook. " */
										/* "That chartbook will handle the alert.",1); */
									}
								}
							}
						}

						// close the alert log file (Outer most file) 
						file.close();
					}
					else
					{
						sc.AddMessageToLog("Error opening file! 2", 1);
					}

				}
				else
				{
					sc.AddMessageToLog("Error opening file! 1", 1);
				}
			}
		}
	}

	if (sc.LastCallToFunction)
	{
		/* sc.AddMessageToLog("Last Call To Function!", 1); */
		FirstTimeStart = 0;
		LastModTime = 0;
		NumberOfLines = 0;
	}

	// Remember the number of alert log files between calls to the study function
	// This is necessary to handle when Sierra Chart is first opened and a new
	// alert log file gets generated.
	if (num_files_memory != number_of_files)
	{
		num_files_memory = number_of_files;
	}
}
