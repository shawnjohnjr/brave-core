import { createReducer } from 'redux-act'
import { init } from '../../actions/new_tab_actions'
import * as Actions from '../../actions/today_actions'

export type BraveTodayState = {
  isFetching: boolean | string
  currentPageIndex: number
  pagedContent: BraveToday.Page[]
  initialSponsor?: BraveToday.Article
  initialHeadline?: BraveToday.Article
  initialDeals?: BraveToday.Deal[]
}

const defaultState: BraveTodayState = {
  isFetching: true,
  currentPageIndex: 0,
  pagedContent: [],
}

const reducer = createReducer<BraveTodayState>({}, defaultState)

export default reducer

reducer.on(init, (state, payload) => ({
  ...state,
  isFetching: true
}))

reducer.on(Actions.errorGettingDataFromBackground, (state, payload) => ({
  ...state,
  isFetching: (payload && payload.error && payload.error.message) || 'Unknown error.',
}))

reducer.on(Actions.dataReceived, (state, payload) => {
  return {
    ...state,
    isFetching: false,
    feed: payload.feed,
    // Reset page index to ask for, even if we have current paged
    // content since feed might be new content.
    currentPageIndex: 0
  }
})

reducer.on(Actions.anotherPageNeeded, (state) => {
  // Add a new page of content to the state
  return {
    ...state,
    currentPageIndex: state.currentPageIndex + 1,
  }
})