/gh_new_procedure\("/ {
	split($0, a, "\"");
	gsub("_", "-", a[2]);
	printf "%s\"%s\"%s\n", a[1], a[2], a[3];
}

! /gh_new_procedure\("/
