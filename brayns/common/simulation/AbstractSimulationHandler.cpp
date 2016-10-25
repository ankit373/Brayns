/* Copyright (c) 2015-2016, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Jafet Villafranca Diaz <jafet.villafrancadiaz@epfl.ch>
 *
 * This file is part of Brayns <https://github.com/BlueBrain/Brayns>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "AbstractSimulationHandler.h"

#include <brayns/common/log.h>

#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace brayns
{

AbstractSimulationHandler::AbstractSimulationHandler()
    : _currentFrame( 0 )
    , _nbFrames( 0 )
    , _frameSize( 0 )
    , _headerSize( 0 )
    , _memoryMapPtr( 0 )
    , _cacheFileDescriptor( -1 )
{

}

AbstractSimulationHandler::~AbstractSimulationHandler()
{
    if( _memoryMapPtr )
    {
        const uint64_t size =
            _headerSize +
            _frameSize * _nbFrames * sizeof(float);
        ::munmap( (void *)_memoryMapPtr, size );
    }
    if( _cacheFileDescriptor != -1 )
        ::close( _cacheFileDescriptor );
}

bool AbstractSimulationHandler::attachSimulationToCacheFile(
    const std::string& cacheFile )
{
    BRAYNS_INFO << "Attaching " << cacheFile << " to current scene" << std::endl;
    _cacheFileDescriptor = open( cacheFile.c_str(), O_RDONLY );
    if( _cacheFileDescriptor == -1 )
    {
        BRAYNS_ERROR << "Failed to open " << cacheFile << std::endl;
        return false;
    }

    struct stat sb;
    if( ::fstat( _cacheFileDescriptor, &sb ) == -1 )
    {
        BRAYNS_ERROR << "Failed to get stats from " << cacheFile << std::endl;
        return false;
    }

    _memoryMapPtr = ::mmap(
        0, sb.st_size, PROT_READ, MAP_PRIVATE, _cacheFileDescriptor, 0 );
    if( _memoryMapPtr == MAP_FAILED )
    {
        _memoryMapPtr = 0;
        BRAYNS_ERROR << "Failed to attach " << cacheFile << std::endl;
        ::close( _cacheFileDescriptor );
        return false;
    }

    _headerSize = 2 * sizeof( uint64_t );

    strncpy((char *)&_nbFrames, (char *)_memoryMapPtr, sizeof( uint64_t ));
    strncpy((char *)&_frameSize, ((char *)_memoryMapPtr + sizeof( uint64_t )), sizeof( uint64_t ));

    BRAYNS_INFO << "Nb Frames: " << _nbFrames << std::endl;
    BRAYNS_INFO << "Frame size: " << _frameSize << std::endl;

    BRAYNS_INFO << "Successfully attached to " << cacheFile << std::endl;
    return true;
}

void AbstractSimulationHandler::writeHeader( std::ofstream& stream )
{
    stream.write( ( char* )&_nbFrames, sizeof( uint64_t ));
    stream.write( ( char* )&_frameSize, sizeof( uint64_t ));
}

void AbstractSimulationHandler::writeFrame(
    std::ofstream& stream,
    const floats& values )
{
    stream.write( ( char* )values.data(), values.size() * sizeof(float) );
}

}