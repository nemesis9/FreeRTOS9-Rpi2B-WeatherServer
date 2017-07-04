
@cpuinfo.s


.text
    .global cpuinfo_getSystemCtlReg
    .global cpuinfo_setSystemCtlReg
    .global cpuinfo_getMainIDReg
    .global cpuinfo_getIDMMFR0
    .global cpuinfo_getSysCntrFreq
    .global cpuinfo_getLowerPhysCnt
    .global cpuinfo_getUpperPhysCnt
    .global cpuinfo_getPMEnReg
    .global cpuinfo_setPMEnReg
    .global cpuinfo_getPMCR
    .global cpuinfo_setPMCR
    .global cpuinfo_getPMCounter
    .global cpuinfo_setPMCounter
    .global cpuinfo_getStackPtr
    .type cpuinfo_getSystemCtlReg, function
    .type cpuinfo_setSystemCtlReg, function
    .type cpuinfo_getMainIDReg, function
    .type cpuinfo_getIDMMFR0, function
    .type cpuinfo_getSysCntrFreq, function
    .type cpuinfo_getLowerPhysCnt, function
    .type cpuinfo_getUpperPhysCnt, function
    .type cpuinfo_getPMEnReg, function
    .type cpuinfo_setPMEnReg, function
    .type cpuinfo_getPMCR, function
    .type cpuinfo_setPMCR, function
    .type cpuinfo_getPMCounter, function
    .type cpuinfo_setPMCounter, function
    .type cpuinfo_getStackPtr, function


cpuinfo_getStackPtr:
    stmfd r13!, {r4-r12, r14}
    mov r0, sp
    ldmfd r13!, {r4-r12, pc}

cpuinfo_getSystemCtlReg:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c1, c0, 0
    ldmfd r13!, {r4-r12, pc}


cpuinfo_setSystemCtlReg:
    stmfd r13!, {r4-r12, r14}
    mcr p15, 0, r0, c1, c0, 0
    ldmfd r13!, {r4-r12, pc}


cpuinfo_getMainIDReg:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c0, c0, 0
    ldmfd r13!, {r4-r12, pc}

cpuinfo_getIDMMFR0:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c0, c1, 4
    ldmfd r13!, {r4-r12, pc}

@get system counter freq
cpuinfo_getSysCntrFreq:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c14, c0, 0
    ldmfd r13!, {r4-r12, pc}

@get system counter count (lower)
cpuinfo_getLowerPhysCnt:
    stmfd r13!, {r4-r12, r14}
    mrrc p15, 0, r0, r1, c14
    ldmfd r13!, {r4-r12, pc}

@get system counter count (upper)
cpuinfo_getUpperPhysCnt:
    stmfd r13!, {r4-r12, r14}
    mrrc p15, 0, r1, r0, c14
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Enable Reg
cpuinfo_getPMEnReg:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c9, c12, 1
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Enable Reg
cpuinfo_setPMEnReg:
    stmfd r13!, {r4-r12, r14}
    mcr p15, 0, r0, c9, c12, 1
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Ctrl Reg
cpuinfo_getPMCR:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c9, c12, 0
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Ctrl Reg
cpuinfo_setPMCR:
    stmfd r13!, {r4-r12, r14}
    mcr p15, 0, r0, c9, c12, 0
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Counter Reg
cpuinfo_getPMCounter:
    stmfd r13!, {r4-r12, r14}
    mrc p15, 0, r0, c9, c13, 0
    ldmfd r13!, {r4-r12, pc}

@Performance Monitor Counter Reg
cpuinfo_setPMCounter:
    stmfd r13!, {r4-r12, r14}
    mcr p15, 0, r0, c9, c13, 0
    ldmfd r13!, {r4-r12, pc}


@ needed anymore ?
@cpwait:
@     mrc  p15, 0, r0, c2, c0, 0
@     mov r0, r0
@     mov pc, lr
