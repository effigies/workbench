#ifndef __SYSTEMUTILITIES_H__
#define __SYSTEMUTILITIES_H__

/*LICENSE_START*/ 
/* 
 *  Copyright 1995-2002 Washington University School of Medicine 
 * 
 *  http://brainmap.wustl.edu 
 * 
 *  This file is part of CARET. 
 * 
 *  CARET is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version. 
 * 
 *  CARET is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 * 
 *  You should have received a copy of the GNU General Public License 
 *  along with CARET; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 */ 


#include <stdint.h>

#include <AString.h>

namespace caret {

class SystemBacktrace
{
#ifdef CARET_OS_WINDOWS
#else // CARET_OS_WINDOWS
    void* m_callstack[1024];
    int m_numFrames;
#endif // CARET_OS_WINDOWS
public:
    SystemBacktrace();
    AString toSymbolString() const;
    friend class SystemUtilities;
};

/**
 * Methods to help out with files and directories.
 */
class SystemUtilities {

private:
    SystemUtilities();

public:
    virtual ~SystemUtilities();

public:
    static AString getBackTrace();
    
    static void getBackTrace(std::vector<AString>& backTraceOut);

    static void getBackTrace(SystemBacktrace& backTraceOut);

    static AString getTempDirectory();

    static AString getUserName();

    static AString getYear();
    
    static AString getDate();

    static AString getTime();

    static AString getDateAndTime();

    static bool isWindowsOperatingSystem();

    static bool isMacOperatingSystem();

    static int32_t getNumberOfProcessors();

    static AString createUniqueID();
    
    static void unitTest(std::ostream& stream,
                         const bool isVerbose);

    static bool testRelativePath(
                    const AString& otherPath,
                    const AString& myPath,
                    const AString& correctResult);

    static AString relativePath(
                    const AString& otherPathIn,
                    const AString& myPathIn);

    static void setHandlersForUnexpected();
    
    static AString systemCurrentDirectory();
    
    static void sleepSeconds(const float numberOfSeconds);
};

} // namespace

#endif // __SYSTEMUTILITIES_H__
