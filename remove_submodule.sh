if [ $# -eq 1 ]; then
	if [ -d $1 ]; then
		echo "Removing submodule $1"
		git submodule deinit -f -- $1
		rm -rf .git/modules/$1
		git rm -f $1
	else
		echo "Path does not exist."
	fi
else
	echo "Execute with path to submodule as argument."
fi
