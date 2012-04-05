#========================================================
# R statistics system grapher for PA4
#========================================================

#========================================================
# determine page from PC 
#========================================================
PAGESIZE <- 128
page <- function (pc) { as.integer(pc/PAGESIZE) } 

#========================================================
# read whole history from file 'output.csv' 
#========================================================
allHistory<- function(x, y, z) UseMethod('allHistory')

allHistory.NULL <- function(x) { allHistory('output.csv') } 

# read results from file 
allHistory.character <- function(file) { 
  out <- read.csv(file, header=FALSE)
  out <- data.frame(time=out$V1, proc=out$V2, pid=out$V3, kind=out$V4, pc=out$V5, comment=out$V6)
  class(out) <- c('allHistory', class(out))
  out 
} 

# scope results in time 
allHistory.allHistory <- function(all, low=0, high=1e25) { 
  sel <- (all$time >= low) & (all$time <= high)
  out <- data.frame(time=all$time[sel], proc=all$proc[sel], 
	pid=all$pid[sel], kind=all$kind[sel], 
	pc=all$pc[sel], comment=all$comment[sel])
  class(out) <- c('allHistory', class(out))
  out
}

#========================================================
# read pages history from file 'pages.csv' 
#========================================================
allPages<- function(x, y, z) UseMethod('allPages')

allPages.NULL <- function(x) { allPages('pages.csv') } 

# read results from file 
allPages.character <- function(file) { 
  out <- read.csv(file, header=FALSE)
  out <- data.frame(time=out$V1, proc=out$V2, page=out$V3, pid=out$V4, kind=out$V5, comment=out$V6)
  class(out) <- c('allPages', class(out))
  out 
} 

# scope results in time 
allPages.allPages <- function(pl, low=0, high=1e25) { 

#   # determine states before window
#   times <- c(); procs <- c(); pages <- c()
#   pids <- c(); kinds <- c(); comments <- c()
#   for (p in c(1:20)) { # processes 
#     for (q in c(1:20)) { # pages 
# 	# boolean bit field 
# 	sel <- (pl$time<low) & (pl$proc==p) & (pl$page==q) 
#         temp <- pl$time[sel]
#         if (length(temp)>0) { 
# 	    times[[length(times)+1]] <- temp[[length(temp)]]
# 	    temp <- pl$proc[sel]
# 	    procs[[length(procs)+1]] <- temp[[length(temp)]]
# 	    temp <- pl$page[sel]
# 	    pages[[length(pages)+1]] <- temp[[length(temp)]]
# 	    temp <- pl$pid[sel]
# 	    pids[[length(pids)+1]] <- temp[[length(temp)]]
# 	    temp <- pl$kind[sel]
# 	    kinds[[length(kinds)+1]] <- temp[[length(temp)]]
# 	    temp <- pl$comment[sel]
# 	    comments[[length(comments)+1]] <- as.character(temp[[length(temp)]])
#         } 
#     } 
#   } 
#   ord <- order(times,procs,pages) 
#   before <- data.frame(time=times[ord], proc=procs[ord], page=pages[ord], 
# 	pid=pids[ord], kind=kinds[ord], comment=comments[ord]) 
# 
#   # determine states after window
#   times <- c(); procs <- c(); pages <- c()
#   pids <- c(); kinds <- c(); comments <- c()
#   for (p in c(1:20)) { # processes 
#     for (q in c(1:20)) { # pages 
# 	# boolean bit field 
# 	sel <- (pl$time>high) & (pl$proc==p) & (pl$page==q) 
#         temp <- pl$time[sel]
#         if (length(temp)>0) { 
# 	    times[[length(times)+1]]       <- temp[[1]]
# 	    temp <- pl$proc[sel]
# 	    procs[[length(procs)+1]]       <- temp[[1]]
# 	    temp <- pl$page[sel]
# 	    pages[[length(pages)+1]]       <- temp[[1]]
# 	    temp <- pl$pid[sel]
# 	    pids[[length(pids)+1]]         <- temp[[1]]
# 	    temp <- pl$kind[sel]
# 	    kinds[[length(kinds)+1]]       <- temp[[1]]
# 	    temp <- pl$comment[sel]
# 	    comments[[length(comments)+1]] <- as.character(temp[[1]])
#         } 
#     } 
#   } 
#   ord <- order(times,procs, pages) 
#   after <- data.frame(time=times[ord], proc=procs[ord], page=pages[ord], 
# 	pid=pids[ord], kind=kinds[ord], comment=comments[ord]) 

# now construct the output
  sel <- ((pl$time >= low) & (pl$time <= high)) 
  out <- data.frame(
      time=pl$time[sel], 
      proc=pl$proc[sel], 
      page=pl$page[sel], 
      pid=pl$pid[sel], 
      kind=pl$kind[sel], 
      comment=pl$comment[sel]
  )
#   out <- data.frame(
#       time=c(before$time,pl$time[sel],after$time), 
#       proc=c(before$proc,pl$proc[sel],after$proc), 
#       page=c(before$page,pl$page[sel],after$page), 
#       pid=c(before$pid,pl$pid[sel],after$pid), 
#       kind=c(before$kind,pl$kind[sel],after$kind), 
#       comment=c(before$comment,pl$comment[sel],after$comment)
#   )
  class(out) <- c('allPages', class(out))
  out
}

#========================================================
# paging history for one process 
#========================================================
procPages<- function(x, y, z, w) UseMethod('procPages')
procPages.allPages <- function(pl, proc, low=0, high=1e25) { 
    sel <- (pl$proc==proc) & (pl$time>=low) & (pl$time<=high) 
    out <- data.frame(time=pl$time[sel], proc=pl$proc[sel], page=pl$page[sel], 
	pid=pl$pid[sel], kind=pl$kind[sel], comment=pl$comment[sel]) 
    class(out) <- c('procPages', class(out))
    out
} 

procPages.procPages <- function(pl, low=0, high=1e25) { 
    sel <- (pl$time>=low) & (pl$time<=high) 
    out <- data.frame(time=pl$time[sel], proc=pl$proc[sel], page=pl$page[sel], 
	pid=pl$pid[sel], kind=pl$kind[sel], comment=pl$comment[sel]) 
    class(out) <- c('procPages', class(out))
    out
} 

#========================================================
# paging history for one job
#========================================================
jobPages<- function(x, y, z, w) UseMethod('jobPages')
jobPages.allPages <- function(pl, pid, low=0, high=1e25) { 
    sel <- (pl$pid==pid) & (pl$time>=low) & (pl$time<=high) 
    out <- data.frame(time=pl$time[sel], proc=pl$proc[sel], page=pl$page[sel], 
	pid=pl$pid[sel], kind=pl$kind[sel], comment=pl$comment[sel]) 
    class(out) <- c('jobPages', class(out))
    out
} 

jobPages.jobPages <- function(pl, low=0, high=1e25) { 
    sel <- (pl$time>=low) & (pl$time<=high) 
    out <- data.frame(time=pl$time[sel], proc=pl$proc[sel], page=pl$page[sel], 
	pid=pl$pid[sel], kind=pl$kind[sel], comment=pl$comment[sel]) 
    class(out) <- c('jobPages', class(out))
    out
} 
#========================================================
# a processor history is obtained from a full history 
# by filtering by processor number 
#========================================================
procHistory <- function(x, y, z, w) UseMethod('procHistory')

# extract a processor record from global history
procHistory.allHistory <- function(all, proc, low=0, high=1e25) { 
  sel <- (all$proc==proc) & (all$time >= low) & (all$time <= high)
  out <- data.frame(time=all$time[sel], proc=all$proc[sel], 
        pid=all$pid[sel], kind=all$kind[sel], 
	pc=all$pc[sel],comment=all$comment[sel])
  class(out) <- c('procHistory', class(out))
  out 
} 

# limit a processor history to a specific time 
procHistory.procHistory <- function(ph, low=0, high=1e25) { 
  sel <- (ph$time >= low) & (ph$time <= high)
  out <- data.frame(time=ph$time[sel], proc=ph$proc[sel], 
        pid=ph$pid[sel], kind=ph$kind[sel], 
        pc=ph$pc[sel],comment=ph$comment[sel])
  class(out) <- c('procHistory', class(out))
  out 
} 

#========================================================
# a job history is obtained from a full history by 
# filtering by job number 0-39
#========================================================
jobHistory <- function(x, y, z, w) UseMethod('jobHistory')

# extract a job from global history
jobHistory.allHistory <- function(all, pid, low=0, high=1e25) { 
  sel <- (all$pid==pid) & (all$time >= low) & (all$time <= high)
  out <- data.frame(time=all$time[sel], proc=all$proc[sel], pid=all$pid[sel], 
        kind=all$kind[sel],pc=all$pc[sel],comment=all$comment[sel])
  class(out) <- c('jobHistory', class(out))
  out 
} 

# limit a job to a specific time 
jobHistory.jobHistory <- function(jh, low=0, high=1e25) { 
  sel <- (jh$time >= low) & (jh$time <= high)
  out <- data.frame(time=jh$time[sel], proc=jh$proc[sel], pid=jh$pid[sel], 
	kind=jh$kind[sel],pc=jh$pc[sel],comment=jh$comment[sel])
  class(out) <- c('jobHistory', class(out))
  out 
} 

#========================================================
# smart plotting routines describe how to plot full 
# histories, processor histories, job histories 
# based upon object class. 
#========================================================

# plot a single job
plot.jobHistory <- function(jh, low=0, high=1e25) { 
  toplot <- jobHistory(jh, low, high) 
  plot(c(), xlab='time', ylab='pc', ylim=c(0,max(toplot$pc)), xlim=c(toplot$time[[1]],toplot$time[[length(toplot$time)]]))
  if (! is.na(pages)) lines(jobPages(pages,toplot$pid[[1]],low,high))
  # print(pages)
  lines(toplot$time, toplot$pc, xlab='time', ylab='pc', col='black')
  plotBlocked(toplot, low, high);  
} 

# plot a single processor's results (all jobs) 
plot.procHistory <- function(ph, low=0, high=1e25) { 
  toplot <- procHistory(ph, low, high) 
  plot(c(), , xlab='time', ylab='pc', ylim=c(0,max(toplot$pc)), xlim=c(toplot$time[[1]],toplot$time[[length(toplot$time)]]))
  if (!is.na(pages)) lines(procPages(pages,toplot$proc[[1]],low, high))
  lines(toplot$time, (toplot$pc), xlab='time', ylab='pc', col='black')
  plotBlocked(ph, low, high);  
} 

# plot all job histories.
plot.allHistory <- function(all, low=0, high=1e25) { 
    toplot <- allHistory(all, low, high) 
    # create a selected region from this! 
    plot(c(), type='l', xlab='time', ylab='process', 
	  xlim=c(1,toplot$time[[length(toplot$time)]]), ylim=c(0,41))
    x <- array()
    y <- array()
    for (job in c(0:39)) { 
       jh <- jobHistory(toplot, job, low, high) 
       if (length(jh$time)>0) { 
	   x[[length(x)+1]] <- jh$time[[1]] 
	   x[[length(x)+1]] <- jh$time[[length(jh$time)]] 
	   x[[length(x)+1]] <- NA
	   y[[length(y)+1]] <- job
	   y[[length(y)+1]] <- job
	   y[[length(y)+1]] <- NA
       } 
    } 
    lines(x,y,col='black')
    x <- array()
    y <- array()
    for (job in c(0:39)) { 
      jh <- jobHistory(toplot, job, low, high) 
      if (length(jh$time)>0) { 
	for (index in c(1:(length(jh$time)-1))) { 
	  if (jh$comment[[index]]=='blocked' 
	   && jh$comment[[index+1]]=='unblocked') { 
	      x[[length(x)+1]] <- jh$time[[index]]
	      x[[length(x)+1]] <- jh$time[[index+1]]
	      x[[length(x)+1]] <- NA
	      y[[length(y)+1]] <- job
	      y[[length(y)+1]] <- job
	      y[[length(y)+1]] <- NA
	  } 
	} 
      } 
    } 
    lines(x,y,col='red')
} 

#========================================================
# subsystem plots page state on block graph
#========================================================
plot.procPages <- function (pl, low=0, high=1e25) { 
    toplot <- procPages(pl, low, high) 
    plot(c(), xlab='time', ylab='memory', ylim=c(0,max(toplot$page)*128), xlim=c(toplot$time[[1]],toplot$time[[length(toplot$time)]]))
    lines(toplot, low, high)
} 

lines.procPages <- function (pl, low=0, high=1e25) { 
    toplot <- procPages(pl, low, high) 

    tot <- max(toplot$page)
    refs <- array(dim=c(tot+1)) 
    coms <- array(dim=c(tot+1)) 
    for (i in c(1:(tot+1))) { 
	refs[[i]] = 0
	coms[[i]] = 'out'
    } 
    xleft <- c()
    xright <- c()
    ybottom <- c()
    ytop <- c()
    cols <- c()
    for (i in c(1:length(toplot$time))) { 
	time <- toplot$time[[i]]
	page <- toplot$page[[i]]
	if (!is.na(refs[[page+1]])) { 
	    oldtime <- refs[[page+1]]; 
	    oldcomm <- coms[[page+1]]; 
	    xleft[[length(xleft)+1]] = oldtime
	    xright[[length(xright)+1]] = time
	    ybottom[[length(ybottom)+1]] = page*128
	    ytop[[length(ytop)+1]] = (page+1)*128
	    if (oldcomm=='in') { 
		cols[[length(cols)+1]] = rgb(.9,1,.9)
	    } else if (oldcomm=='out') { 
		cols[[length(cols)+1]] = rgb(1,.9,.9)
	    } else if (oldcomm=='coming') { 
		cols[[length(cols)+1]] = rgb(1,1,.9) 
	    } else { # if (oldcomm=='going') 
		cols[[length(cols)+1]] = rgb(1,.9,1)
	    } 
	} 
	refs[[page+1]]=toplot$time[[i]]
	coms[[page+1]]=as.character(toplot$comment[[i]])
    } 
    end <- toplot$time[[length(toplot$time)]]
    # plot to end of line 
    for (page in c(0:19)) { 
	if (!is.na(refs[[page+1]])) { 
	    oldtime <- refs[[page+1]]; 
	    oldcomm <- coms[[page+1]]; 
	    xleft[[length(xleft)+1]] = oldtime
	    xright[[length(xright)+1]] = end 
	    ybottom[[length(ybottom)+1]] = page*128
	    ytop[[length(ytop)+1]] = (page+1)*128
	    if (oldcomm=='in') { 
		cols[[length(cols)+1]] = rgb(.9,1,.9)
	    } else if (oldcomm=='out') { 
		cols[[length(cols)+1]] = rgb(1,.9,.9)
	    } else if (oldcomm=='coming') { 
		cols[[length(cols)+1]] = rgb(1,1,.9)
	    } else { # if (oldcomm=='going') 
		cols[[length(cols)+1]] = rgb(1,.9,1)
	    } 
        } 
    } 
    rect(xleft=xleft, ybottom=ybottom, xright=xright, ytop=ytop, col=cols, border=NA)
    # title(paste('pages for processor ',toplot$proc[[1]]))
} 

plot.jobPages <- function (pl, low=0, high=1e25) { 
    toplot <- jobPages(pl, low, high) 
    plot(c(), xlab='time', ylab='memory', ylim=c(0,max(toplot$page)*128), xlim=c(toplot$time[[1]],toplot$time[[length(toplot$time)]]))
    lines(toplot, low, high)
} 

lines.jobPages <- function (pl, low=0, high=1e25) { 
    toplot <- jobPages(pl, low, high) 

    tot <- max(toplot$page)
    refs <- array(dim=c(tot+1)) 
    coms <- array(dim=c(tot+1)) 
    for (i in c(1:(tot+1))) { 
	refs[[i]] = 0
	coms[[i]] = 'out'
    } 
    xleft <- c()
    xright <- c()
    ybottom <- c()
    ytop <- c()
    cols <- c()
    for (i in c(1:length(toplot$time))) { 
	time <- toplot$time[[i]]
	page <- toplot$page[[i]]
	if (!is.na(refs[[page+1]])) { 
	    oldtime <- refs[[page+1]]; 
	    oldcomm <- coms[[page+1]]; 
	    xleft[[length(xleft)+1]] = oldtime
	    xright[[length(xright)+1]] = time
	    ybottom[[length(ybottom)+1]] = page*128
	    ytop[[length(ytop)+1]] = (page+1)*128
	    if (oldcomm=='in') { 
		cols[[length(cols)+1]] = rgb(.9,1,.9)
	    } else if (oldcomm=='out') { 
		cols[[length(cols)+1]] = rgb(1,.9,.9)
	    } else if (oldcomm=='coming') { 
		cols[[length(cols)+1]] = rgb(1,1,.9) 
	    } else { # if (oldcomm=='going') 
		cols[[length(cols)+1]] = rgb(1,.9,1)
	    } 
	} 
	refs[[page+1]]=toplot$time[[i]]
	coms[[page+1]]=as.character(toplot$comment[[i]])
    } 
    end <- toplot$time[[length(toplot$time)]]
    # plot to end of line 
    for (page in c(0:19)) { 
	if (!is.na(refs[[page+1]])) { 
	    oldtime <- refs[[page+1]]; 
	    oldcomm <- coms[[page+1]]; 
	    xleft[[length(xleft)+1]] = oldtime
	    xright[[length(xright)+1]] = end 
	    ybottom[[length(ybottom)+1]] = page*128
	    ytop[[length(ytop)+1]] = (page+1)*128
	    if (oldcomm=='in') { 
		cols[[length(cols)+1]] = rgb(.9,1,.9)
	    } else if (oldcomm=='out') { 
		cols[[length(cols)+1]] = rgb(1,.9,.9)
	    } else if (oldcomm=='coming') { 
		cols[[length(cols)+1]] = rgb(1,1,.9)
	    } else { # if (oldcomm=='going') 
		cols[[length(cols)+1]] = rgb(1,.9,1)
	    } 
        } 
    } 
    rect(xleft=xleft, ybottom=ybottom, xright=xright, ytop=ytop, col=cols, border=NA)
    # title(paste('pages for processor ',toplot$proc[[1]]))
} 

#========================================================
# subsystem plots blockages in red over grey background
#========================================================
plotBlocked <- function(x, y, z) UseMethod('plotBlocked')
# print red blocked times over one job history
plotBlocked.jobHistory <- function(jh, low=0, high=1e25) { 
  toplot <- jobHistory(jh, low, high) 
  x <- array()
  y <- array()
  for (i in c(1:(length(toplot$time)-1))) { 
    if (toplot$comment[[i]]=='blocked' && toplot$comment[[i+1]]=='unblocked') { 
	x[[length(x)+1]] <- toplot$time[[i]]
	x[[length(x)+1]] <- toplot$time[[i+1]]
	x[[length(x)+1]] <- NA
	y[[length(y)+1]] <- (toplot$pc[[i]])
	y[[length(y)+1]] <- (toplot$pc[[i]])
	y[[length(y)+1]] <- NA
    } 
  } 
  lines(x,y,col='red'); 
} 

# print red blocked times over one processor history
plotBlocked.procHistory <- function(ph, low=0, high=1e25) { 
  toplot <- procHistory(ph, low, high) 
  x <- array()
  y <- array()
  for (i in c(1:(length(toplot$time)-1))) { 
    if (toplot$comment[[i]]=='blocked' && toplot$comment[[i+1]]=='unblocked') { 
	x[[length(x)+1]] <- toplot$time[[i]]
	x[[length(x)+1]] <- toplot$time[[i+1]]
	x[[length(x)+1]] <- NA
	y[[length(y)+1]] <- (toplot$pc[[i]])
	y[[length(y)+1]] <- (toplot$pc[[i]])
	y[[length(y)+1]] <- NA
    } 
  } 
  lines(x,y,col='red'); 
} 

#========================================================
# interactive subsystem describes how to give more info
#========================================================
interact <- function(x, y, z, w) UseMethod('interact')

# zoom in on one job from full history 
interact.allHistory <- function (all, low=0, high=1e25) { 
  while(TRUE) { 
    plot(all, low, high) 
    title("click on a timeline to graph the PC for the job")
    which <- locator(1)
    job <- as.integer(which$y[[1]]+0.5)
    if ((job >= 0) && (job <= 39)) { 
	interact(jobHistory(all,job))
    } else { 
	break
    } 
  } 
} 

# plot one job interactively
interact.jobHistory <- function(jh, low=0, high=1e25) { 
  plot(jh,low,high)
  title("click anywhere to return to overall view") 
  which <- locator(1)
} 

#========================================================
# use the system to visualize results
#========================================================

# read file from disk into full history 
all <- allHistory()
pages <- try(allPages(), silent=TRUE)
if (class(pages)[[1]] == 'try-error') { 
    pages <- NA 
} 

# go interactive and show results 
interact(all) 
