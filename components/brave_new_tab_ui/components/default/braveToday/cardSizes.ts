// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css } from 'brave-ui/theme'
import {
  Block as StandardBlock,
  Heading as StandardHeading,
  Image as StandardImage,
  Text as StandardText,
  Time as StandardTime,
  PublisherLogo as StandardPublisherLogo
} from './default'

export const Large = styled(StandardBlock.withComponent('article'))`
  padding: 0;
`

export const Medium = styled(Large)`
  padding: 0;
`

export const Small = styled(Large)`
  width: 100%;
  min-height: 310px;
`

export const DealsCard = styled(StandardBlock)`

`


export const Content = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  padding: 25px 35px;
`

export const ImageFrame = styled('figure')`
  margin: 0;
  width: 100%;
  position: relative;
  padding-top: 67%;
  display: block;
  height: auto;
  overflow: hidden;
`

export const ListImageFrame = styled(ImageFrame)`
  height: 100%;
  padding-top: 0;
`

export const Image = styled(StandardImage)`
  display: block;
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
  background-color: rgba(188,188,188,0.2);
`

export const Heading = styled(StandardHeading)`
  a {
    display: block;
    color: inherit;
    text-decoration: none;
  }
`

export const Text = styled(StandardText)`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 18px;
  line-height: 25px;

  a {
    display: block;
    color: inherit;
    text-decoration: none;
  }
`

export const Time = styled(StandardTime)``

export const PublisherLogo = styled(StandardPublisherLogo)`
  margin: 36px 0 12px;
`

export const ContainerForTwo = styled<{}, 'div'>('div')`
  width: 680px;
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-gap: 30px;
`

export const DealsList = styled('div')`
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  grid-gap: 30px;
`

export const DealItem = styled('a')`
  display: block;
`