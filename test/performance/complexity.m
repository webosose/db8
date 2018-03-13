function analysis(fl, iters)

%clear ;
close all; clc

% read data
data=csvread(fl);
result = data(:,20:size(data,2)) ./ iters;
p=mean(result')';

figure;

hold("on");
xlabel("N objects");
ylabel("Time (ms)");

loglog(data(:, 1), p, ':+');

x = data(:,1); % the independent data set - force
y = p; % the dependent data set - deflection

coefficents = polyfit(x,y,1); %  finds coefficients for a first degree line
Y = polyval(coefficents, x); % use coefficients from above to estimate a new set of y values
loglog(x,Y,'-r'); % plot the original data with * and the estimated line with —
hold("off");

fprintf('Calculate average complexity\n');

ps = length(p);
a1 = p(ps);
a2 = p(ps - 1);

amin = min(a1, a2);
amax = max(a1, a2);

blog = log2(amax / amin);
display('b complexity: ')
display(blog);

end
