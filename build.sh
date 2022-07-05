projname=dict
cd ~
rm $projname
rm out.txt
#echo "/mnt/c/Users/$USER/source/repos/$projname"
#read -n1 -r -p "Press any key to continue..."
cd /mnt/c/Users/$USER/source/repos/$projname
cmake -S . -B ./build/rel -DCMAKE_BUILD_TYPE=Release
cmake --build ./build/rel
cd bin
cp $projname ~
cd ~
./$projname huge.txt out.txt
git diff --no-index out.txt out_huge.txt
