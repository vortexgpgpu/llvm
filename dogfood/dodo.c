
int dodo(int* d, const int* s, int n) {
  int r = 0;
  for (int i = 0; i < n; ++i) {   
    int t = s[i]; 
    if (t == 0)
        goto L_RET;
    if (t == 1)
        goto L_CLEANUP;
    if (t == 255)
        break;
    d[i] = s[i];
    ++r;
  }
  r = 0;
L_CLEANUP:
    ++r;
L_RET:
  return r;
}