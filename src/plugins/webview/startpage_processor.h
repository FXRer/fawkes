
/***************************************************************************
 *  startpage_processor.h - Web request processor for the start page
 *
 *  Created: Thu Feb 12 00:09:16 2009
 *  Copyright  2006-2009  Tim Niemueller [www.niemueller.de]
 *
 *  $Id$
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#ifndef __PLUGINS_WEBVIEW_STARTPAGE_PROCESSOR_H_
#define __PLUGINS_WEBVIEW_STARTPAGE_PROCESSOR_H_

#include "request_processor.h"

namespace fawkes {
  class CacheLogger;
}

class WebStartPageRequestProcessor : public WebRequestProcessor
{
 public:
  WebStartPageRequestProcessor(fawkes::CacheLogger *cache_logger);
  virtual ~WebStartPageRequestProcessor();

  virtual WebReply * process_request(const char *url,
				     const char *method,
				     const char *version,
				     const char *upload_data,
				     unsigned int *upload_data_size,
				     void **session_data);

 private:
  fawkes::CacheLogger *__cache_logger;
};

#endif
