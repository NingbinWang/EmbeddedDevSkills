# Report Template

Use this structure in the final answer. Keep it concise, but do not omit the conclusion.

## Symptom

- What the user observed
- Relevant time window
- Commands or artifacts used

## Evidence By Area

### CPU

- System-wide evidence
- Process-level evidence
- Whether CPU is primary, secondary, or not the main issue

### Memory

- System-wide evidence
- Process-level evidence
- Whether memory is primary, secondary, or not the main issue

### I/O

- System-wide evidence
- Process-level evidence
- Whether I/O is primary, secondary, or not the main issue

### Network

- System-wide evidence
- Process-level evidence
- Whether network is primary, secondary, or not the main issue

## Conclusion

State these three items explicitly:

1. Primary bottleneck: CPU, memory, I/O, or network.
2. Bottleneck layer: kernel space or application space.
3. Triggering process: the main application process, or `none identified` if the issue is not attributable to a user-space process.

## Next Step

- Give the single best next command or remediation direction.
