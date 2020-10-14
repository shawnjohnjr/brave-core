import { createReducer } from 'redux-act'
import { init } from '../../actions/new_tab_actions'
import * as Actions from '../../actions/today_actions'

export type BraveTodayState = {
  // Are we in the middle of checking for new data
  isFetching: boolean | string
  // How many pages have been displayed so far for the current data
  currentPageIndex: number
  // Feed data
  feed?: BraveToday.Feed
  // pagedContent: BraveToday.Page[]
  // initialSponsor?: BraveToday.Article
  // initialHeadline?: BraveToday.Article
  // initialDeals?: BraveToday.Deal[]
}

const defaultState: BraveTodayState = {
  isFetching: true,
  currentPageIndex: 0,
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