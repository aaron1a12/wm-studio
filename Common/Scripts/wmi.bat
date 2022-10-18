@echo off
for /f "skip=1" %%p in ('wmic cpu get loadpercentage') do EXIT /B %%p