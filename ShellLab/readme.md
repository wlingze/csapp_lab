# shell lab

[toc]

# Knowledge pointer

Textbook Chapter 8.

- parent-child process.
- signal processing.
- signal blocking and unblocking.

# Solution

## part1: run the program

First make `tsh` can run the program, use `frok` and `execve`, run the program in the foreground.

```c
pid = fork();
state = parseline(cmdline, argv);

if(!pid){
    if(execve(argv[0], argv, environ) < 0){
        printf("%s: command not found\n", argv[0]);
        exit(0);
    }
}else{
    wait(pid, NULL);
}
```

## part2: background command

Through `SIGCHLD` to realize the foreground command and the background command.

Mentioned in wp, use `waitfd()` for waiting and `sigchld_handle()` for subprocess.

Using the jobs array, adding jobs when performing tasks, deleting the jobs corresponding jobs when receiving the `SIGCHLD` signal, and the `waitfg()` function itself loops until there is no `FG` process.

```c
void eval(char *cmdline){
    // ....
    state = parseline(cmdline, argv) + 1;
    if(!pid){
        if(execve(argv[0], argv, environ) < 0){
            printf("%s: command not found\n", argv[0]);
            exit(0);
        }
    }else{
        addjobs(jobs, pid, state, cmdline);
        if(state == FG){
            waitfg(pid);
        }else{
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }
    // ...
}

void sigchld_handler(int sig) {
  pid_t pid;
  int statue;
  while ((pid = waitpid(-1, &statue, 0)) > 0) {
    deletejob(jobs, pid);
  }
  return;
}

waitfg(pid_t, pid){
    while(getfd(jobs)){
        ;
    }
    return;
}
```

## part3: built-in command

In the `eval` function, first enter the `builtin_cmd()` function to run before `fork`, if it is not a built-in command, then return and call the external command.

```c

void eval(char *cmdline) {
  // ...
  if (!strcmp(cmdline, "\n")) {
    return;
  }

  state = parseline(cmdline, argv) + 1;

  if (builtin_cmd(argv)) {
    return;
  }
  // ...

}

int builtin_cmd(char **argv) {

  if (!strcmp(argv[0], "jobs")) {
    listjobs(jobs);
    return 1;
  } else if (!strcmp(argv[0], "fg")) {
    do_bgfg(argv);
    return 1;
  } else if (!strcmp(argv[0], "bg")) {
    do_bgfg(argv);
    return 1;
  } else if (!strcmp(argv[0], "quit")) {
    exit(0);
    return 1;
  }
  return 0; /* not a builtin command */
}

```

## part4: signal process

For signal process, and foreground process background process switching.

```c
// signal handle
void sigtstp_handler(int sig) {
  pid_t pid = fgpid(jobs);
  if (!pid) {
    return;
  }
  kill(-(pid), SIGTSTP);
  return;
}

void sigint_handler(int sig) {
  pid_t pid = fgpid(jobs);
  if (!pid) {
    return;
  }
  kill(-(pid), SIGINT);
  return;
}

void sigchld_handler(int sig) {
  pid_t pid;
  int statue;
  sigset_t mask_all, prev_all;

  sigfillset(&mask_all);

  while ((pid = waitpid(-1, &statue, WNOHANG | WUNTRACED)) > 0) {
    switch (statue) {
    case 2: { // ctrl-c
      printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid,
             WTERMSIG(statue));
    }
    case 0: { // exit
    }
    case 256: { // execve error
      sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
      deletejob(jobs, pid);
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
      break;
    }

    default: { // ctrl-z
      printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid,
             WSTOPSIG(statue));

      sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
      struct job_t *job = getjobpid(jobs, pid);
      job->state = ST;
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
      break;
    }
    }
  }
  return;
}
```

```c
// do_bgfd()
void do_bgfg(char **argv){
      int tid;
  struct job_t *job;

  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }

  if (strchr(argv[1], '%')) {
    if (sscanf(argv[1], "%%%d", &tid) <= 0) {
      printf("%s: argument must be a PID or %%jobid\n", argv[0]);
      return;
    }
    job = getjobjid(jobs, tid);
    if (!job) {
      printf("%%%d: No such job\n", tid);
      return;
    }
  } else {
    if (sscanf(argv[1], "%d", &tid) <= 0) {
      printf("%s: argument must be a PID or %%jobid\n", argv[0]);
      return;
    }
    job = getjobpid(jobs, tid);
    if (!job) {
      printf("(%d): No such process\n", tid);
      return;
    }
  }

  kill(-(job->pid), SIGCONT);

  if (strstr(argv[0], "bg")) {
    job->state = BG;
    printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
  } else {
    job->state = FG;
    waitfg(job->pid);
  }

  return;
}

```

## part5: blocking signal

At this time, we have to block the signal, otherwise some function may not be realized.

Add blocking at the position where the jobs are increased or decreased, modify `waitfg()` to use `sigsuspend`.

```c
// blocking signal
{
  sigset_t mask_all, prev_all;

  sigfillset(&mask_all);

  sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
  addjob(jobs, pid, state, cmdline);
  sigprocmask(SIG_SETMASK, &prev_all, NULL);
}

// waitfg
void waitfg(pid_t pid) {
  sigset_t mask_all;
  sigemptyset(&mask_all);
  while (fgpid(jobs)) {
    sigsuspend(&mask_all);
  }
  return;
}
```
