function graphit(name)

errorbar (name(:,1) - 2000, name(:, 5), name(:, 6), name(:, 7), 'ro')
hold

errorbar (name(:,1) , name(:, 8), name(:, 9), name(:, 10), 'bs')
hold

legend ('Vdelta standard deviation', 'Vdelta mean', 'Xdelta standard deviation', 'Xdelta mean')
set(gca, 'XTickLabel', [0,50000,100000,150000,200000,250000,300000,350000,400000])
ylabel ('Delta Size Ratio')
xlabel ('To File Size (bytes)')

plot     (name(:,1) - 2000, name(:, 5), 'r-')
hold
hold

plot     (name(:,1) , name(:, 8), 'b-')
hold
hold
