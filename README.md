# Introducere

Procesarea de imagini este un domeniu foarte vast în computer science, aflat în strânsă legătură cu alte domenii precum computer graphics și computer vision.
Cu toții ați auzit de face detection și de device-urile sau aplicațiile care îl integrează printre feature-urile lor și pe acesta. Câteva exemple sunt: focalizarea, înainte de a face o poză, pe smartphone-uri/camere foto, facebook, care îți recomandă să îți tăguiești prietenii din poză, marcându-le fața într-un chenar, etc.
Ne propunem să simulăm, la un nivel simplificat, algoritmul care stă în spatele face detection-ului, având ca use case protejarea identității, prin blur-area fețelor detectate (puteți lua ca exemplu Street View-ul de la Google Maps). De ce simplificat? Deoarece complexitatea algoritmilor care fac asta ar depăși scopul acestui curs și ar fi, de asemenea, și foarte greu de automatizat testarea unei astfel de procesări. 

# Structura formatului BMP

 Vom lucra cu fișiere BMP, deci, cu fișiere binare.

O imagine BMP are următoarea structură:

- un File Header care are următoarele câmpuri:\
    -signature – 2 octeți - literele 'B' și 'M' în ASCII;\
    -file size – 4 octeți – dimensiunea întregului fișier;\
    -reserved – 4 octeți – nefolosit;\
    -offset – 4 octeți – offsetul de la începutul fișierului până la începutului bitmap-ului, adica al matricii de pixeli.
- un Info Header care poate avea structuri diferite, însă noi vom lucra cu cel care se numește BITMAPINFOHEADER. Are următoarele câmpuri:\
    -size – 4 octeți – dimensiunea Info Header-ului, care are o valoare fixă, 40;\
    -width – 4 octeți – lățimea matricii de pixeli (numărul de coloane);\
    -height – 4 octeți – înălțimea matricii de pixeli (numărul de rânduri);\
    -planes – 2 octeți – setat la valoarea fixă 1;\
    -bit count – 2 octeți – numărul de biți per pixel. În cazul nostru va avea mereu valoarea 24, adică reprezentăm fiecare pixel pe 3 octeți, adică cele 3 canale, 
    RGB;\
    -compression – 4 octeți – tipul de compresie. Acest câmp va fi 0;\
    -image size – 4 octeți – se referă la dimensiunea matricii de pixeli, inclusiv padding-ul adăugat (vedeți mai jos);\
    -x pixels per meter – 4 octeți – se referă la rezoluția de printare. Pentru a simplifica puțin tema, veți seta acest câmp pe 0. Nu o să printăm imaginile :).\
    -y pixels per meter – la fel ca mai sus;\
    -colors used – numărul de culori din paleta de culori. Aceasta este o secțiune care va lipsi din imaginile noastre BMP, deoarece ea se află în general imediat 
    după Info Header însă doar pentru imaginile care au câmpul bit count mai mic sau egal cu 8. Prin urmare, câmpul va fi setat pe 0;\
    -colors important – numărul de culori importante. Va fi, de asemenea, setat pe 0, ceea ce înseamnă că toate culorile sunt importante.
- BitMap-ul, care este matricea de pixeli și care ocupă cea mai mare parte din fișier. Trei lucruri trebuiesc menționate despre aceasta:\
    -pixelii propriu-ziși se află într-o matrice de dimensiune height x width, însă ea poate avea o dimensiune mai mare de atât din cauza paddingului. Acest 
    padding este adăugat la sfârșitul fiecărei linii astfel încat fiecare linie să înceapă de la o adresă (offset față de începutul fișierului) multiplu de 4.
    Mare atenție la citire, pentru că acest padding trebuie ignorat (fseek). De asemenea, la scriere va trebui să puneți explicit valoarea 0 pe toți octeții de 
    padding.\
    -este răsturnată, ceea ce înseamnă că prima linie în matrice conține de fapt pixelii din extremitatea de jos a imaginii.\
    -canelele pentru fiecare pixel sunt în ordinea BGR (Blue Green Red).
    
# Cerințe

 Cum simulăm de fapt procedeul de face detection? Știm că în format BMP, o imagine nu este altceva decât o matrice de pixeli. Astfel, a identifica fața unei persoane într-o imagine, la un nivel simplificat, ar însemna identificarea zonei din imagine care are toți pixelii într-un interval dat de un pixel referință și un pixel offset (dați ca input în această temă). O zonă, în acest caz, e definită ca o mulțime de pixeli adiacenți din imagine (sau poziții vecine în matrice). Această zonă nu este necesar să aibă o formă regulată.

Observați cum simplificăm modul în care funcționează face detection-ul în aplicațiile reale, prin faptul că oferim ca input valorile RGB ale unui pixel. Acesta este și motivul pentru care nu vom putea folosi ca input “poze reale”, deoarece culoarea pielii diferă, în realitate, de luminozitatea externă, calitatea camerei foto, etc. Prin urmare, nu vă așteptați la efecte vizuale impresionante.

O să îi spunem cluster zonei din imagine despre care vorbeam mai sus și o să numim un pixel valid dacă se află în intervalul dat de pixelul referință și cel offset. Prin urmare, un pixel dat prin valorile sale RGB (Rv, Gv, Bv), este valid, dacă și numai dacă, dându-se 2 pixeli numiți referință (Rr, Gr, Br) și offset (Ro, Go, Bo), următoarele inegalități sunt adevărate. 

- Cerința 1 – Clustere (30 puncte)\
Să se afișeze în ordine crescătoare dimensiunile clusterelor de pixeli care se află în intervalul determinat de pixelii referință și offset dați ca input. Se vor considera doar clusterele în care se găsesc cel puțin P*Height*Width pixeli, unde P este un procent dat ca input. Cu alte cuvinte, nu ne interesează acele clustere care au prea puțini pixeli. 

- Cerința 2 - Blur (30 puncte)\
Se cere blur-area clusterelor identificate la cerința anterioară. Efectul de blur presupune înlocuirea fiecărui pixel care face parte dintr-un cluster cu media aritmetică a vecinilor săi (stânga, dreapta, jos, sus). 
 
- Cerința 3 – Crop (40 puncte)
Să se decupeze clusterele identificate la prima cerință. Neblurate!
Ne dorim, însă, ca maginile rezultate să aibă o formă regulată, de dreptunghi. Pentru asta, ceea ce va trebui să “tăiați” pentru fiecare cluster este cel mai mic dreptunghi care îl cuprinde. 

# Checker

- test.sh, scriptul de testare, care poate fi rulat în următoarele moduri:\

    -./test.sh sau ./test.sh all- va rula toate testele;\
    -./test.sh n - unde n este un număr între 1 și numărul de teste (2 în prezent, însă se vor adăuga teste noi în zilele următoare) - va rula doar testul n.

- input, director care conține fișierele de intrare.
- ref, director care conține fișierele de ieșire referință.
