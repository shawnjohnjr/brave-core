// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'brave-ui/theme'
import { Block as DefaultBlock} from './default'

export const MediaLink = styled(DefaultBlock)`
  box-sizing: border-box;
  position: relative;
  display: block;
  border-radius: 8px;
  padding: 0;

  img {
    display: block;
    object-fit: cover;
    max-width: 100%;
  }
`
