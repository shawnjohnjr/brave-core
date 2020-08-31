// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { combineReducers } from 'redux'
import * as storage from '../storage/new_tab_storage'

// Reducers
import newTabStateReducer from './new_tab_reducer'
import gridSitesReducer from './grid_sites_reducer'
import binanceReducer from './binance_reducer'
import rewardsReducer from './rewards_reducer'
import geminiReducer from './gemini_reducer'
import bitcoinDotComReducer from './bitcoin_dot_com_reducer'
import cryptoDotComReducer from './cryptoDotCom_reducer'

export const newTabReducers = (state: NewTab.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  state = newTabStateReducer(state, action)
  state = binanceReducer(state, action)
  state = rewardsReducer(state, action)
  state = geminiReducer(state, action)
  state = bitcoinDotComReducer(state, action)
  state = cryptoDotComReducer(state, action)

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export const mainNewTabReducer = combineReducers<NewTab.ApplicationState>({
  newTabData: newTabReducers,
  gridSitesData: gridSitesReducer
})

export const newTabReducer = newTabStateReducer
