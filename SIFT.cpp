#sift-complete.bash
_sift() 
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    if [[ ${cur} == -* ]] ; then
		opts="$(sift --help | sift '\s+(--[\w-]+)(?:\s|=)' --replace '$1' --output-sep ' ')"
		COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
	fi

	case "${prev}" in
		-t|-T|--type|--no-type|--del-type)
			types="$(sift --list-types | sift '^(\w+)\s+:' --replace '$1' --output-sep ' ')"
			COMPREPLY=( $(compgen -W "${types}" -- ${cur}) )
			return 0;;
	esac

	_filedir
	return 0
}
complete -F _sift sift

#matching
// sift
#include "textflag.h"

// func countNewlines(input []byte, length int) int
TEXT ·countNewlines(SB),NOSPLIT,$0-40
	MOVQ input+0(FP), SI
	MOVQ length+24(FP), CX

	XORQ AX, AX
	CMPQ CX, $16
	JL small

	// X0 = 16 x 0x00
	// X1 = 16 x 0x01
	// X2 = 16 x '\n'
	// X3 = input buffer

	PXOR X0, X0

	MOVQ $0x0101010101010101, DX
	MOVQ DX, X1
	PUNPCKLQDQ X1, X1

	MOVQ $0x0a0a0a0a0a0a0a0a, DX
	MOVQ DX, X2
	PUNPCKLQDQ X2, X2

	PXOR X6, X6

	CMPQ CX, $128*16
	JL bigloop

hugeloop:
	MOVQ $128, BX
	PXOR X4, X4

hugeloop_inner:
	MOVOU (SI), X3
	PCMPEQB X2, X3
	PAND X1, X3
	PADDD X3, X4
	ADDQ $16, SI
	DECQ BX
	JNZ hugeloop_inner

	PSADBW X0, X4
	PADDQ X4, X6

	SUBQ $128*16, CX
	CMPQ CX, $128*16
	JGE hugeloop

bigloop:
	CMPQ CX, $16
	JL finish
	MOVOU (SI), X3
	PCMPEQB X2, X3
	PAND X1, X3
	PSADBW X0, X3
	PADDQ X3, X6
	SUBQ $16, CX
	ADDQ $16, SI
	JMP bigloop

finish:
	PXOR X7, X7
	MOVHLPS X6, X7
	PADDQ X6, X7
	MOVQ X7, AX

small:
	TESTQ CX, CX
	JZ done
	CMPB (SI), $'\n'
	JNE nonewline
	INCQ AX
nonewline:
	INCQ SI
	DECQ CX
	JMP small

done:
	MOVQ AX, ret+32(FP)
	RET


// func bytesToLower(input []byte, output []byte, length int)
TEXT ·bytesToLower(SB),NOSPLIT,$0-56
	MOVQ input+0(FP), SI
	MOVQ output+24(FP), DI
	MOVQ length+48(FP), CX

    // X0 = 16 x 0x00
    // X1 = 16 x 0x25 (127 - 'Z')
    // X2 = 16 x 0x65 (127 - 26)
    // X3 = 16 x 0x20 ('a' - 'A')
    // X4 = input buffer

	XORQ AX, AX
	CMPQ CX, $16
	JL small

	PXOR X0, X0

	MOVQ $0x2525252525252525, DX
	MOVQ DX, X1
	PUNPCKLQDQ X1, X1

	MOVQ $0x6565656565656565, DX
	MOVQ DX, X2
	PUNPCKLQDQ X2, X2

	MOVQ $0x2020202020202020, DX
	MOVQ DX, X3
	PUNPCKLQDQ X3, X3

	XORQ BX, BX
bigloop:
	MOVOU (SI)(BX*8), X4
	MOVOU X4, X5
	PADDB X1, X5
	PCMPGTB X2, X5
	PAND X3, X5
	PADDB X5, X4
	MOVOU X4, (DI)(BX*8)

	ADDQ $2, BX
	SUBQ $16, CX
	CMPQ CX, $16
	JGE bigloop

	TESTQ CX, CX
	JZ done

	SUBQ $16, CX
	ADDQ CX, SI
	ADDQ CX, DI
	MOVOU (SI)(BX*8), X4
	MOVOU X4, X5
	PADDB X1, X5
	PCMPGTB X2, X5
	PAND X3, X5
	PADDB X5, X4
	MOVOU X4, (DI)(BX*8)
	JMP done

small:
	CLD
loop_small:
	TESTQ CX, CX
	JZ done
	MOVB (SI), AX
	CMPB AX, $'Z'
	JA skip
	CMPB AX, $'A'
	JB skip
	ADDB $0x20, AX
	MOVB AX, (DI)
	INCQ SI
	INCQ DI
	DECQ CX
	JMP loop_small
skip:
	MOVSB
	DECQ CX
	JMP loop_small

done:
	RET

#output
// sift
package main

import (
	"fmt"
	"path/filepath"
	"strings"
)

func resultHandler() {
	for result := range global.resultsChan {
		if options.TargetsOnly {
			fmt.Println(result.target)
			continue
		}
		global.totalTargetCount++
		result.applyConditions()
		printResult(result)
	}
	global.resultsDoneChan <- struct{}{}
}

func writeOutput(format string, a ...interface{}) {
	output := fmt.Sprintf(format, a...)
	_, err := global.outputFile.Write([]byte(output))
	if err != nil {
		errorLogger.Fatalln("cannot write to output file:", err)
	}
}

func printFilename(filename string, delim string) {
	if options.ShowFilename == "on" && !options.GroupByFile {
		if options.OutputUnixPath {
			filename = filepath.ToSlash(filename)
		}
		writeOutput(global.termHighlightFilename+"%s"+global.termHighlightReset+delim, filename)
	}
}

func printLineno(lineno int64, delim string) {
	if options.ShowLineNumbers {
		writeOutput(global.termHighlightLineno+"%d"+global.termHighlightReset+delim, lineno)
	}
}

func printColumnNo(m *Match) {
	if options.ShowColumnNumbers {
		writeOutput("%d"+options.FieldSeparator, m.start-m.lineStart+1)
	}
}

func printByteOffset(m *Match) {
	if options.ShowByteOffset {
		if options.OnlyMatching {
			writeOutput("%d"+options.FieldSeparator, m.start)
		} else {
			writeOutput("%d"+options.FieldSeparator, m.lineStart)
		}
	}
}

// printMatch prints the context after the previous match, the context before the match and the match itself
func printMatch(match Match, lastMatch Match, target string, lastPrintedLine *int64) {
	var matchOutput = match.line

	if !options.InvertMatch {
		if options.Replace != "" {
			matchOutput = match.match
			var matchTest string
			if options.IgnoreCase {
				tmp := []byte(match.match)
				for i := 0; i < len(tmp); i++ {
					bytesToLower(tmp, tmp, len(tmp))
				}
				matchTest = string(tmp)
			} else {
				matchTest = match.match
			}

			var res []byte
			for _, re := range global.matchRegexes {
				submatchIndexes := re.FindAllStringSubmatchIndex(matchTest, -1)
				if len(submatchIndexes) > 0 {
					for _, subIndex := range submatchIndexes {
						res = re.ExpandString(res, options.Replace, matchOutput, subIndex)
					}
					break
				}
			}

			matchOutput = string(res)
			if options.OutputLimit > 0 {
				var end int
				if options.OutputLimit > len(matchOutput) {
					end = len(matchOutput)
				} else {
					end = options.OutputLimit
				}
				matchOutput = matchOutput[0:end]
			}
		} else {
			// replace option not used
			if options.OutputLimit > 0 {
				var end int
				if options.OutputLimit > len(matchOutput) {
					end = len(matchOutput)
				} else {
					end = options.OutputLimit
				}
				matchOutput = matchOutput[0:end]
			}
			if options.Color == "on" {
				start := match.start - match.lineStart
				end := match.end - match.lineStart
				if int(end) <= len(matchOutput) {
					matchOutput = matchOutput[0:end] + global.termHighlightReset + matchOutput[end:]
					matchOutput = matchOutput[0:start] + global.termHighlightMatch + matchOutput[start:]
				}
			}
		}
	}

	// print contextAfter of the previous match
	contextBlockIncomplete := false
	if lastMatch.contextAfter != nil {
		contextLines := strings.Split(*lastMatch.contextAfter, "\n")
		for index, line := range contextLines {
			var lineno int64
			if options.Multiline {
				multilineLineCount := len(strings.Split(lastMatch.line, "\n")) - 1
				lineno = lastMatch.lineno + int64(index) + 1 + int64(multilineLineCount)
			} else {
				lineno = lastMatch.lineno + int64(index) + 1
			}
			// line is not part of the current match
			if lineno < match.lineno {
				printFilename(target, "-")
				printLineno(lineno, "-")
				writeOutput("%s\n", line)
				*lastPrintedLine = lineno
			} else {
				contextBlockIncomplete = true
			}
		}
	}
	if (lastMatch.contextAfter != nil || match.contextBefore != nil) && !contextBlockIncomplete {
		if match.lineno-int64(options.ContextBefore) > *lastPrintedLine+1 {
			// at least one line between the contextAfter of the previous match and the contextBefore of the current match
			fmt.Fprintln(global.outputFile, "--")
		}
	}

	// print contextBefore of the current match
	if match.contextBefore != nil {
		contextLines := strings.Split(*match.contextBefore, "\n")
		for index, line := range contextLines {
			lineno := match.lineno - int64(len(contextLines)) + int64(index)
			if lineno > *lastPrintedLine {
				printFilename(target, "-")
				printLineno(lineno, "-")
				writeOutput("%s\n", line)
				*lastPrintedLine = lineno
			}
		}
	}

	// print current match
	if options.Multiline {
		lines := strings.Split(match.line, "\n")
		if len(lines) > 1 && options.Replace == "" {
			firstLine := lines[0]
			lastLine := lines[len(lines)-1]
			firstLineOffset := match.start - match.lineStart
			lastLineOffset := int64(len(lastLine)) - (match.lineEnd - match.end)

			// first line of multiline match with partial highlighting
			printFilename(target, options.FieldSeparator)
			printLineno(match.lineno, options.FieldSeparator)
			printColumnNo(&match)
			printByteOffset(&match)
			writeOutput("%s%s%s%s\n", firstLine[0:firstLineOffset], global.termHighlightMatch,
				firstLine[firstLineOffset:len(firstLine)], global.termHighlightReset)

			// lines 2 upto n-1 of multiline match with full highlighting
			for i := 1; i < len(lines)-1; i++ {
				line := lines[i]
				printFilename(target, options.FieldSeparator)
				printLineno(match.lineno+int64(i), options.FieldSeparator)
				writeOutput("%s%s%s\n", global.termHighlightMatch, line, global.termHighlightReset)
			}

			// last line of multiline match with partial highlighting
			printFilename(target, options.FieldSeparator)
			printLineno(match.lineno+int64(len(lines))-1, options.FieldSeparator)
			writeOutput("%s%s%s%s%s", global.termHighlightMatch, lastLine[0:lastLineOffset],
				global.termHighlightReset, lastLine[lastLineOffset:len(lastLine)], options.OutputSeparator)
			*lastPrintedLine = match.lineno + int64(len(lines)-1)
		} else {
			// single line output in multiline mode or replace option used
			printFilename(target, options.FieldSeparator)
			printLineno(match.lineno, options.FieldSeparator)
			printColumnNo(&match)
			printByteOffset(&match)
			writeOutput("%s%s", matchOutput, options.OutputSeparator)
			*lastPrintedLine = match.lineno + int64(len(lines)-1)
		}
	} else {
		// single line output
		printFilename(target, options.FieldSeparator)
		printLineno(match.lineno, options.FieldSeparator)
		printColumnNo(&match)
		printByteOffset(&match)
		writeOutput("%s%s", matchOutput, options.OutputSeparator)
		*lastPrintedLine = match.lineno
	}
}

// printResult prints results using printMatch and handles various output options.
func printResult(result *Result) {
	var matchCount int64
	target := result.target
	matches := result.matches
	if options.FilesWithoutMatch {
		if len(matches) == 0 {
			writeOutput("%s\n", target)
			global.totalResultCount++
		}
		return
	}
	if options.FilesWithMatches && !options.Count {
		if len(matches) > 0 {
			writeOutput("%s\n", target)
			global.totalMatchCount++
			global.totalResultCount++
		}
		return
	}
	if options.Count {
		matchCount = int64(len(matches))
		if options.Limit != 0 && matchCount > options.Limit {
			matchCount = options.Limit
		}
		if result.streaming {
		countingMatchesLoop:
			for matches := range result.matchChan {
				matchCount += int64(len(matches))
				if options.Limit != 0 && matchCount >= options.Limit {
					matchCount = options.Limit
					break countingMatchesLoop
				}
			}
		}
		if options.FilesWithMatches {
			if matchCount > 0 {
				writeOutput("%s"+options.FieldSeparator+"%d\n", target, matchCount)
			}
		} else {
			if options.ShowFilename == "on" {
				writeOutput("%s"+options.FieldSeparator, target)
			}
			writeOutput("%d\n", matchCount)
		}
		global.totalMatchCount += matchCount
		if matchCount > 0 {
			global.totalResultCount++
		}
		return
	}

	if len(matches) == 0 {
		return
	}

	// print separator between file results if this is not the first result
	if global.totalMatchCount > 0 {
		if options.GroupByFile {
			fmt.Fprintln(global.outputFile, "")
		} else {
			if options.ContextBefore > 0 || options.ContextAfter > 0 {
				fmt.Fprintln(global.outputFile, "--")
			}
		}
	}

	if result.isBinary && !options.BinarySkip && !options.BinaryAsText {
		filename := result.target
		if options.OutputUnixPath {
			filename = filepath.ToSlash(filename)
		}
		writeOutput("Binary file matches: %s\n", filename)
		global.totalMatchCount++
		global.totalResultCount++
		return
	}

	if options.GroupByFile {
		filename := result.target
		if options.OutputUnixPath {
			filename = filepath.ToSlash(filename)
		}
		writeOutput(global.termHighlightFilename+"%s\n"+global.termHighlightReset, filename)
	}

	var lastPrintedLine int64 = -1
	var lastMatch Match

	// print contextBefore of first match
	if m := matches[0]; m.contextBefore != nil {
		contextLines := strings.Split(*m.contextBefore, "\n")
		for index, line := range contextLines {
			lineno := m.lineno - int64(len(contextLines)) + int64(index)
			printFilename(result.target, "-")
			printLineno(lineno, "-")
			writeOutput("%s\n", line)
			lastPrintedLine = lineno
		}
	}

	// print matches with their context
	lastMatch = matches[0]
	for _, match := range matches {
		printMatch(match, lastMatch, result.target, &lastPrintedLine)
		lastMatch = match
		matchCount++
		if options.Limit != 0 && matchCount >= options.Limit {
			break
		}
	}
	if result.streaming {
	matchStreamLoop:
		for matches := range result.matchChan {
			for _, match := range matches {
				printMatch(match, lastMatch, result.target, &lastPrintedLine)
				lastMatch = match
				matchCount++
				if options.Limit != 0 && matchCount >= options.Limit {
					break matchStreamLoop
				}
			}
		}
	}

	// print contextAfter of last match
	if lastMatch.contextAfter != nil {
		contextLines := strings.Split(*lastMatch.contextAfter, "\n")
		for index, line := range contextLines {
			var lineno int64
			if options.Multiline {
				multilineLineCount := len(strings.Split(lastMatch.line, "\n")) - 1
				lineno = lastMatch.lineno + int64(index) + 1 + int64(multilineLineCount)
			} else {
				lineno = lastMatch.lineno + int64(index) + 1
			}
			printFilename(result.target, "-")
			printLineno(lineno, "-")
			writeOutput("%s\n", line)
			lastPrintedLine = lineno
		}
	}

	global.totalMatchCount += matchCount
	global.totalResultCount++
}
