import { createReducer } from 'redux-act'
import { init } from '../../actions/new_tab_actions'
import * as Actions from '../../actions/today_actions'

const defaultState: NewTab.BraveTodayState = {
  isFetching: true,
}

const reducer = createReducer<NewTab.BraveTodayState>({}, defaultState)

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
    feed: payload.feed
  }
})