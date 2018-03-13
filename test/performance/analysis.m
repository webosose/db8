function analysis(fl, iters)

%clear ; 
close all; clc

data=csvread(fl);
result = data(:,20:size(data,2)) ./ iters;

fprintf('min: ');
min(result')
fprintf('\n');

fprintf('mean: ');
mean(result')
fprintf('\n');

fprintf('median: ');
median(result')
fprintf('\n');

fprintf('range: ');
range(result')
fprintf('\n');

fprintf('max:');
max(result')
fprintf('\n');

fprintf('std:');
std(result')
fprintf('\n');

fprintf('Press any key to plot t(n)');
pause;
figure;
hold("on");
xlabel("N objects");
ylabel("Time (ms)");

p=mean(result')';
plot(data(:, 1), p, "-+");
hold("off");

fprintf('Press any key to plot hist(i)');
pause;
figure;
xlabel("frequency");
ylabel("value");

for i=1:size(result,1)
    figure(i);
    hist(result(i,:));
endfor

end
