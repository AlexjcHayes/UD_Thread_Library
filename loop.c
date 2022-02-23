int main() {
int val = 17;
int pid, status = 0;
if (pid = fork()) // note that = is not the same as ==
waitpid(pid, &status, 0);
else
exit(val);
val++;
printf("%d %d\n", val, WEXITSTATUS(status));
return val;
}