(echo abc > temp) && echo new 

(echo def || cat temp > temp1) && echo mno > temp

(cat temp > temp2 || echo pqr > temp2) || echo xyz > temp2

echo ghi > temp2 | cat temp2

echo 123 > temp;echo 456 > temp

cat temp > temp1
