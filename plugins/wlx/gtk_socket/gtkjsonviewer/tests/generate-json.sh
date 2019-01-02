echo '['
for ((i = 1; i < $1; i++)); do
    echo {'"id": '"$i"', "name" : "XYZ"},'
done
echo '{"id":' $i, '"name" : "XYZ" }]'
