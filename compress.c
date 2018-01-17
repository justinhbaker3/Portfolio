
int compress(char * s){

	if (s == NULL) return -1;
	char * cur = s;
	size_t len = strlen(s);
	for (int i = 0; i < len;){
		int count = 1;
		char temp = s[i];
		if (temp < 'a' || temp > 'z'){
			return -1;
		}
		for (int j = i+1; j < len; j++){
			if (s[j] != temp){
				break;
			}
			else{
				count++;
			}
		}
		if (count <= 2){
			memmove(cur, s+i, count);
			cur += count;
		}
		else{
			cur += sprintf(cur, "%d", count);
			cur[0] = temp;
			cur++;
		}
		i += count;
	}
	cur[0] = '\0';
	return strlen(s);
}