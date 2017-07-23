src=$1
dst=$2
mkdir -p $(dirname $dst)
air="${src}.air"
xcrun -sdk macosx metal $src -o $air
xcrun -sdk macosx metallib $air -o $dst
rm $air
