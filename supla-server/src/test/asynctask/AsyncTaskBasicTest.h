/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef ASYNCTASKBASIC_TEST_H_
#define ASYNCTASKBASIC_TEST_H_

#include "AsyncTaskTest.h"
#include "asynctask_queue.h"
#include "doubles/asynctask/AsyncTaskMock.h"
#include "doubles/asynctask/AsyncTaskThreadPoolMock.h"
#include "gtest/gtest.h"  // NOLINT

namespace testing {

class AsyncTaskBasicTest : public AsyncTaskTest {
 protected:
  AsyncTaskMock *task;

 public:
  AsyncTaskBasicTest();
  virtual ~AsyncTaskBasicTest();
  void SetUp();
};

} /* namespace testing */

#endif /* ASYNCTASKBASIC_TEST_H_ */
