function graphit(name)

plot     (name(:,1), name(:, 2), 'ro')
hold
plot     (name(:,1) , name(:, 3), 'bs')

legend ('Vdelta', 'Xdelta')
ylabel ('Delta Size (bytes)')
xlabel ('To File Size (bytes)')
