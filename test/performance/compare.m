function analysis(fl, fm, fcd)

%clear ;
close all; clc

data1 = csvread(fl);
data2 = csvread(fm);

file1=fopen(fcd,'w');
i=100;

while((any(data1(:,1) == i)) && (any(data2(:,1) == i)) )
{
fprintf(file1,'%d,%3.4f,%3.4f\n',mean(data1(data1(:,1)==i,:)),mean(data2(data2(:,1)==i,2)));
i= 2*i;
};
end

fclose(file1);

plotData = csvread(fcd);
figure;
hold("on");
xlabel("N objects");
ylabel("Times(ms)");
plot(plotData(:,1),plotData(:,2),"r-+");
plot(plotData(:,1),plotData(:,3),"b-+");
hold("off");


end
