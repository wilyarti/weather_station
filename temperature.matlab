% Read temperature data from a ThingSpeak channel for three seperate days 
% and visualize the data in a single plot using the PLOT function. 

% Channel ID to read data from 
readChannelID = 645847; 
% Temperature Field ID 
TemperatureFieldID = 4; 

% Channel Read API Key 
% If your channel is private, then enter the read API 
% Key between the '' below: 
readAPIKey = ''; 

%sdfsdf
% Specify date range
dateRange = [datetime('today')-days(7),datetime('today')];
% Read data including the timestamp, and channel information.
[data,time,channelInfo] = thingSpeakRead(readChannelID,'Fields',1:4,...
                          'DateRange',dateRange);
% Create variables to store different sorts of data
temperatureData = data(:,1);
humidityData = data(:,3);
pressureData = data(:,2);
batteryData = data(:,4);


% Create a day range vector
dayRange = day(dateRange(1):dateRange(2));
% Pre-allocate matrix
weatherData = zeros(length(dayRange),24);

% Generate temperature 3D bar chart
% Get temperature per whole clock for each day
for m = 1:length(dayRange) % Loop over all days
    for n = 1:24 % Loop over 24 hours
        if any(day(time)==dayRange(m) & hour(time)==n); % Check if data exist for this specific time
            hourlyData = temperatureData((day(time)==dayRange(m) & hour(time)==n)); % Pull out the hourly temperature from the matrix
            weatherData(m,n) = hourlyData(1); % Assign the temperature at the time closest to the whole clock
        end
    end
end

% Plot
figure
h = bar3(datenum(dateRange(1):dateRange(2)), weatherData);
for k = 1:length(h) % Change the face color for each bar
    h(k).CData = h(k).ZData;
    h(k).FaceColor = 'interp';
end
title('Temperature Distribution')
xlabel('Hour of Day')
ylabel('Date')
datetick('y','mmm dd') % Change the Y-Tick to display specified date format
ax = gca;
ax.XTick = 1:24
ax.XTickLabels = 1:24
ax.YTickLabelRotation = 30; % Rotate label for better display
colorbar % Add a color bar to indicate the scaling of color
