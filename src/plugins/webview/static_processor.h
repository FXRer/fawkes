
/***************************************************************************
 *  static_processor.h - Web request processor for static files
 *
 *  Created: Mon Oct 13 23:41:34 2008
 *  Copyright  2006-2008  Tim Niemueller [www.niemueller.de]
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

#ifndef __PLUGINS_WEBVIEW_STATIC_PROCESSOR_H_
#define __PLUGINS_WEBVIEW_STATIC_PROCESSOR_H_

#include "request_processor.h"
#include <cstdlib>

class WebStaticRequestProcessor : public WebRequestProcessor
{
 public:
  WebStaticRequestProcessor(const char *baseurl,
			    const char *htdocs_dir);
  virtual ~WebStaticRequestProcessor();

  virtual WebReply * process_request(const char *url,
				     const char *method,
				     const char *version,
				     const char *upload_data,
				     unsigned int *upload_data_size,
				     void **session_data);

 private:
  char *__baseurl;
  size_t __baseurl_len;
  char *__htdocs_dir;
};

#endif