dir=$1
xcrun -sdk macosx metal "${dir}/gui.metal" -o "${dir}/gui.air"
xcrun -sdk macosx metallib "${dir}/gui.air" -o "${dir}/gui.metallib"
